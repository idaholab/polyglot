#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <limits.h>


extern char **environ;


/* PIPE WAITPID TRICK *********************************************************/
// (c.f. https://stackoverflow.com/questions/282176)

int selfpipe[2];

void selfpipe_sigh(int n)
{
    int save_errno = errno;
    (void)write(selfpipe[1], "",1);
    errno = save_errno;
}

void selfpipe_setup(void)
{
    static struct sigaction act;
    if (pipe(selfpipe) == -1) { abort(); }

    fcntl(selfpipe[0],F_SETFL,fcntl(selfpipe[0],F_GETFL)|O_NONBLOCK);
    fcntl(selfpipe[1],F_SETFL,fcntl(selfpipe[1],F_GETFL)|O_NONBLOCK);
    memset(&act, 0, sizeof(act));
    act.sa_handler = selfpipe_sigh;
    sigaction(SIGCHLD, &act, NULL);
}

int selfpipe_waitpid(int child, int *status)
{
    static char dummy[128];
    struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(selfpipe[0], &rfds);
    errno = 0;
    if (select(selfpipe[0] + 1, &rfds, NULL, NULL, &tv) > 0) {
        while (read(selfpipe[0], dummy, sizeof(dummy)) > 0);
        errno = 0;
        if (waitpid(child, status, WNOHANG) != -1)
            return child;
    }
    return -1;
}

/******************************************************************************/

#define RESTORE_POS         "\033[u"
#define SAVE_POS            "\033[s"
#define CLEAR_LINE          "\033[0K"
#define FMT_INDENT          "%.*s"
#define FMT_STATUS          "%s "
#define FMT_TIMER_RUNNING   "\033[34m[%02ld:%02ld]\033[0m "
#define FMT_TIMER_DONE      "\033[2m[%02ld:%02ld]\033[0m "
#define FMT_MSG             "%s "

const char* status_working = "\033[34m•\033[0m";
const char* status_success = "\033[32m✔\033[0m";
const char* status_failure = "\033[31m✘\033[0m";

const char* indent_str = "                                ";
sigset_t oldsigs;

const char *prog;
int indent = 0;
const char* msg = "";
char* path;
char* args[4];
const char *outpath = NULL;
const char *errpath = NULL;


void print_error(const char *fmt, ...)
{
    va_list ap;
    int err = errno;
    va_start(ap, fmt);
    fprintf(stderr, "\n");
    fprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ": %s\n", strerror(err));
}


static char shell[PATH_MAX+1] = { 0 };

char* find_shell(const char *name)
{
    const char* envpath;
    char *path, *p, *dir;

    if (*shell)
        return shell;

    if (name == NULL)
        name = "bash";

    if ((envpath = getenv("PATH")) == NULL) {
        errno = EINVAL;
        return NULL;
    }
    if ((path = malloc(strlen(envpath) + 1)) == NULL) {
        return NULL;
    }
    strcpy(path, envpath);

    p = path;
    while ((dir = strtok(p, ":"))) {
        snprintf(shell, sizeof(shell), "%s/%s", dir, name);
        if (access(shell, X_OK) == 0) {
            p = shell;
            goto done;
        }
        p = NULL;
    }

    memset(shell, 0, sizeof(shell));
    errno = ENOENT;

done:
    free(path);
    return p;
}


static char* script = NULL;
static char* pos = NULL;
static size_t script_size = 0;

int ensure_script_size(size_t size)
{
    char *newptr;

    while ((script ? strlen(script) : 0) + size >= script_size) {
        if ((newptr = realloc(script, script_size + 1024)) == NULL)
            return -1;
        script = newptr;
        script_size += 1024;
    }
    if (!pos)
        pos = script;
    return 0;
}

void append_to_script(const char *fmt, ...)
{
    size_t n;
    va_list ap;
    va_start(ap, fmt);
    n = vsnprintf(pos, script_size - (pos - script), fmt, ap);
    va_end(ap);
    if (n >= 0)
        pos += n;
}


int split_env_arg(const char* arg)
{
    const char* flags = NULL;
    const char* name;
    const char* value;
    size_t n;

    value = strchr(arg, '=');
    name = strchr(arg, ':');
    if (name && ((value == NULL) || (name < value))) {
        name += 1;
        flags = arg;
    } else {
        name = arg;
    }
    if (ensure_script_size(strlen(arg) + 16) == -1)
        return -1;
    append_to_script("declare%s%.*s %.*s%s; ",
        flags ? " -" : "",
        flags ? name - flags - 1 : 0,
        flags ? flags : "",
        value ? value - name : strlen(name),
        name,
        value ? value : "");
    return 0;
}


int do_child()
{
    int newout, newerr;

    // open our out/err files
    if (errpath && *errpath) {
        if ((newerr = open(errpath, O_WRONLY|O_CREAT|O_APPEND, 0664)) == -1) {
            print_error("open(%s) failed", errpath);
            return EXIT_FAILURE;
        }
        // move stderr -> new error handle
        close(STDERR_FILENO);
        dup2(newerr, STDERR_FILENO);
        close(newerr);
    }
    if (outpath && *outpath) {
        if ((newout = open(outpath, O_WRONLY|O_CREAT|O_APPEND, 0664)) == -1) {
            print_error("open(%s) failed", outpath);
            return EXIT_FAILURE;
        }
        // move stdout -> new output handle
        close(STDOUT_FILENO);
        dup2(newout, STDOUT_FILENO);
        close(newout);
    }
    // restore the original sigmask
    sigprocmask(SIG_SETMASK, &oldsigs, NULL);
    // then finally execute our desired process
    execve(path, args, environ);
    return EXIT_FAILURE;
}

int do_parent(int child)
{
    sigset_t newsigs;
    int status;
    time_t start, now, elapsed;

    // setup the pipe-trick
    selfpipe_setup();
    // figure out when we started
    start = time(NULL);
    elapsed = 0;
    // print initial message
    fprintf(stderr, SAVE_POS FMT_INDENT FMT_STATUS
            FMT_TIMER_RUNNING FMT_MSG CLEAR_LINE,
            indent, indent_str,
            status_working,
            (long)elapsed / 60, (long)elapsed % 60,
            msg);
    fflush(stdout);
    // unblock SIGCHLD so our pipe works as expected
    sigemptyset(&newsigs);
    sigaddset(&newsigs, SIGCHLD);
    sigprocmask(SIG_UNBLOCK, &newsigs, NULL);
    // loop waiting on the child; we implement it this way because
    while (1) {
        if (selfpipe_waitpid(child, &status) == -1) {
            switch (errno)
            {
            // no error (or interrupt) is okay, print an update
            case 0:
            case EINTR:
                // update our timer
                now = time(NULL);
                elapsed = now - start;
                // print an updated timer
                fprintf(stderr, RESTORE_POS SAVE_POS FMT_INDENT FMT_STATUS
                        FMT_TIMER_RUNNING FMT_MSG CLEAR_LINE,
                        indent, indent_str,
                        status_working,
                        (long)elapsed / 60, (long)elapsed % 60,
                        msg);
                fflush(stdout);
                break;
            // a real error, it seems, so die
            default:
                print_error("waiting for child failed");
                return EXIT_FAILURE;
            }
        // if the child exited, we should exit
        } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
            // update our timer a final time
            now = time(NULL);
            elapsed = now - start;
            // and print our final status message
            fprintf(stderr, RESTORE_POS FMT_INDENT FMT_STATUS
                    FMT_TIMER_DONE FMT_MSG CLEAR_LINE "\n",
                    indent, indent_str,
                    WIFEXITED(status) && (WEXITSTATUS(status) == 0)
                        ? status_success
                        : status_failure,
                    (long)elapsed / 60, (long)elapsed % 60,
                    msg);
            return WIFEXITED(status)
                        ? WEXITSTATUS(status)
                        : -WTERMSIG(status);
        }
    }
    // if we made it here, it's a failure
    return EXIT_FAILURE;
}


static const char* opts = "hm:i:O:E:e:";
static struct option long_options[] = {
    {"help",    no_argument,        0, 'h'},
    {"message", required_argument,  0, 'm'},
    {"indent",  required_argument,  0, 'i'},
    {"stdout",  required_argument,  0, 'O'},
    {"stderr",  required_argument,  0, 'E'},
    {"env",     required_argument,  0, 'e'},
    {0, 0, 0, 0},
};


void print_help()
{
#define P(x, ...) printf(x "\n", ##__VA_ARGS__);
P("Usage: command_timer [-h] [-m<msg>] [-i<ind>] [-O<out>] [-E<err>]")
P("                     [-e[<attrs>:]<name>[=<value>] ...] -- <cmd> ...")
P()
P("Positional arguments:")
P("  <cmd> ...             command to time")
P()
P("Optional arguments:")
P("  -h/--help             print this help messsage")
P("  -m/--message <msg>    the message to display with the timer")
P("  -i/--indent <ind>     how many spaces to indent the timer")
P("  -O/--stdout <out>     file to append command stdout to")
P("  -E/--stderr <err>     file to append command stderr to")
P("  -e/--env [<attrs>:]<name>[=<value>]")
P("                        declare an environment variable in the command")
P("                        environment, optionally setting attrs and value")
P()
#undef P
}


int main(int argc, char **argv)
{
    prog = argv[0];
    int longidx = 0, c;
    while ((c = getopt_long(argc, argv, opts, long_options, NULL)) != -1) {
        switch (c)
        {
        case 'h':
            print_help();
            return EXIT_SUCCESS;
        case 'm':
            msg = optarg;
            break;
        case 'i':
            indent = atoi(optarg);
            break;
        case 'O':
            outpath = optarg;
            break;
        case 'E':
            errpath = optarg;
            break;
        case 'e':
            if (split_env_arg(optarg) == -1) {
                print_error("failed to add env to script");
                return EXIT_FAILURE;
            }
            break;
        default:
            return EXIT_FAILURE;
        }
    }
    if (optind >= argc) {
        fprintf(stderr, "not enough arguments\n");
        return EXIT_FAILURE;
    }

    for (; optind < argc; ++optind) {
        if (ensure_script_size(strlen(argv[optind]) + 1) == -1) {
            print_error("failed to generate script");
            return EXIT_FAILURE;
        }
        append_to_script(" %s", argv[optind]);
    }
    
    if ((path = find_shell(NULL)) == NULL) {
        print_error("failed to find shell");
        return EXIT_FAILURE;
    }
    args[0] = "bash";
    args[1] = "-c";
    args[2] = script;
    args[3] = NULL;

    pid_t pid;
    sigset_t newsigs;

    // block all signals immediately so we can handle them later
    sigfillset(&newsigs);
    sigprocmask(SIG_BLOCK, &newsigs, &oldsigs);
    // fork and handle each of the resulting options
    switch ((pid = fork()))
    {
    case -1:
        print_error("fork() failed");
        break;
    case 0:
        do_child();
        break;
    default:
        return do_parent(pid);
    }
    // if we made it here, then it's a failure
    return EXIT_FAILURE;
}

