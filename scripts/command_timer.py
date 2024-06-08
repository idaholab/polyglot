#!/usr/bin/env python3

# This file is part of Polyglot.
#
# Copyright (C) 2024, Battelle Energy Alliance, LLC ALL RIGHTS RESERVED
#
# Polyglot is free software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation; either version 3, or (at your option) any later version.
#
# Polyglot is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this software; if not see <http://www.gnu.org/licenses/>.

if __name__ != '__main__':
    raise ImportError('script cannot sanely be imported')

################################################################################

import argparse
import os
import re
import signal
import subprocess
import sys
import threading
import time
import shlex
from types import SimpleNamespace as NS

################################################################################

epilog = '''
File handle redirects are in the same format as used for bash commands.

Environment variables should be specified in the format:
    [<bash-attrs>:]<varname>=<value>

Formatting variables:
    {i}             indent, as specified by -i
    {raw_msg}       raw input message, as specified by -m
    {msg}           formatted message, per --format-message
    {min}           minutes elapsed
    {sec}           seconds elapsed since last minute
    {timer}         formatted timer, per --format[-zero]-timer
    {status}        status, per --status-*
    {F}             ANSI escape formatting meta-object (see below)

ANSI escape formatting:
    {F.*}:          X/reset; B/bold, XB/Xbold; D/dim, XD/Xdim;
                    I/italic, XI/Xitalic; U/uline, XU/Xuline;
                    reverse, Xreverse; strike, Xstrike; fg.*; bg.*
    {F.(fg|bg).*}:  X/default, K/black, R/red, G/green, Y/yellow,
                    B/blue, M/magenta, C/cyan, W/white, index, rgb
'''

parser = argparse.ArgumentParser(
    formatter_class = argparse.RawDescriptionHelpFormatter,
    epilog = epilog,
)
grpbas = parser.add_argument_group('basic options')
grpbas.add_argument('-m', '--message',
                    metavar = '<string>',
                    default = '',
                    help = 'message to prefix to timer')
grpbas.add_argument('-l', '--leave',
                    action  = 'store_true',
                    help    = 'leave final at exit')
grpbas.add_argument('-L', '--leave-on-failure',
                    action  = 'store_true',
                    help    = 'leave final at exit if child failed')
grpbas.add_argument('-d', '--delay',
                    metavar = '<seconds>',
                    type    = float,
                    default = 0.0,
                    help    = 'delay after child start until showing format')
grpbas.add_argument('-I', '--interval',
                    metavar = '<seconds>',
                    type    = float,
                    default = 0.25,
                    help    = 'interval between updates')
grpfmt = parser.add_argument_group('formatting options')
grpfmt.add_argument('-i', '--indent',
                    metavar = '<count>',
                    type    = int,
                    default = 0,
                    help    = 'spaces to indent by')
grpfmt.add_argument('--format-standard',
                    metavar = '<format>',
                    default = '{i}{F.fg.B}{status} {F.fg.B}{timer}{F.X} {msg} ',
                    help    = 'custom line format')
grpfmt.add_argument('--format-initial',
                    metavar = '<format>',
                    default = '{i}{F.fg.B}{status} {F.fg.B}{timer}{F.X} {msg} ',
                    help    = 'custom initial line format')
grpfmt.add_argument('--format-message',
                    metavar = '<format>',
                    default = '{raw_msg}',
                    help    = 'custom message format')
grpfmt.add_argument('--format-final',
                    metavar = '<format>',
                    default = '{i}{status} {F.D}{timer}{F.X} {msg}\n',
                    help    = 'custom final line format')
grpfmt.add_argument('--format-timer',
                    metavar = '<format>',
                    default = '{min:02d}:{sec:02d}',
                    help    = 'custom timer format')
grpfmt.add_argument('--format-zero-timer',
                    metavar = '<format>',
                    default = ' ··· ',
                    help    = 'custom zeroed initial/final timer format')
grpfmt.add_argument('--status-running',
                    metavar = '<format>',
                    default = '•',
                    help    = 'custom running status format')
grpfmt.add_argument('--status-success',
                    metavar = '<format>',
                    default = '{F.fg.G}✔{F.X}',
                    help    = 'custom success status format')
grpfmt.add_argument('--status-failure',
                    metavar = '<format>',
                    default = '{F.fg.R}✘{F.X}',
                    help    = 'custom failure status format')
grprar = parser.add_argument_group('less-common options')
grprar.add_argument('-p', '--plain',
                    action  = 'store_true',
                    help    = "don't write fancy output")
grprar.add_argument('--hide-cursor',
                    action  = 'store_true',
                    help    = 'hide cursor while showing timer')
grpcmd = parser.add_argument_group('command options')
grpcmd.add_argument('-f', '--fh',
                    metavar = '<&-expr>',
                    dest    = 'handles',
                    action  = 'append',
                    default = [],
                    help    = 'redirect a file for the child')
grpcmd.add_argument('-e', '--var',
                    metavar = '<envvar>',
                    dest    = 'env_vars',
                    action  = 'append',
                    default = [],
                    help    = 'define a variable in the child environment')
grpcmd.add_argument('command',
                    metavar = '<cmd>',
                    help    = 'command to run as child')
grpcmd.add_argument('args',
                    metavar = '<arg>',
                    nargs   = '*',
                    help    = 'arguments to child command')
args = parser.parse_args()

################################################################################

CSI         = '\033['
RESTORE_POS = f'{CSI}u'
SAVE_POS    = f'{CSI}s'
CLEAR_LINE  = f'{CSI}0K'
HIDE_CURSOR = f'{CSI}?25l'
SHOW_CURSOR = f'{CSI}?25h'
SGR         = lambda *a: '{}{}m'.format(CSI, ';'.join(map(str, a)))

F = NS(
    fg = NS(
        K = SGR(30), black   = SGR(30),
        R = SGR(31), red     = SGR(31),
        G = SGR(32), green   = SGR(32),
        Y = SGR(33), yellow  = SGR(33),
        B = SGR(34), blue    = SGR(34),
        M = SGR(35), magenta = SGR(35),
        C = SGR(36), cyan    = SGR(36),
        W = SGR(37), white   = SGR(37),
                     index   = f'{CSI}38;5;',
                     rgb     = f'{CSI}38;2;',
        X = SGR(39), default = SGR(39),
    ),
    bg = NS(
        K = SGR(40), black   = SGR(40),
        R = SGR(41), red     = SGR(41),
        G = SGR(42), green   = SGR(42),
        Y = SGR(43), yellow  = SGR(43),
        B = SGR(44), blue    = SGR(44),
        M = SGR(45), magenta = SGR(45),
        C = SGR(46), cyan    = SGR(46),
        W = SGR(47), white   = SGR(47),
                     index   = f'{CSI}48;5;',
                     rgb     = f'{CSI}48;2;',
        X = SGR(49), default = SGR(49),
    ),
    X  = SGR(0),  reset    = SGR(0),
    B  = SGR(1),  bold     = SGR(1),
    D  = SGR(2),  dim      = SGR(2),
    I  = SGR(3),  italic   = SGR(3),
    U  = SGR(4),  uline    = SGR(4),
                  reverse  = SGR(7),
                  strike   = SGR(9),
    XB = SGR(22), Xbold    = SGR(22),
    XD = SGR(22), Xdim     = SGR(22),
    XI = SGR(23), Xitalic  = SGR(23),
    XU = SGR(24), Xuline   = SGR(24),
                  Xreverse = SGR(27),
                  Xstrike  = SGR(29),
)

################################################################################

FH_REDIR = re.compile(r'^(?:(0)<|([12])?>(>?))(?:&(\d+)|(.+))$')

status = None
sync_cond = threading.Condition()
timer_lock = threading.Lock()

def do_command():
    global status

    # acquire the signaling lock immediately, then notify the main thread that
    # we have it acquired so that the timer thread can be started
    with timer_lock:
        with sync_cond:
            sync_cond.notify()

        handles = {
            0: sys.stdin.buffer,
            1: sys.stdout.buffer,
            2: sys.stderr.buffer,
        }
        for h in args.handles:
            if (m := FH_REDIR.match(h)) is None:
                raise Exception(f'invalid redirect: {h!r}')
            stdin, stdout, append, fd, path = m.groups()
            if stdin:
                key = 0
                mode = 'rb'
            else:
                key = int(stdout) if stdout else 1
                mode = 'ab' if append else 'wb'
            if fd:
                fd = int(fd)
                handles[key] = handles[fd] if fd in handles \
                               else os.fdopen(fd, mode)
            else:
                handles[key] = open(path, mode)

        commands = []
        for var in args.env_vars:
            name, value = var.split('=', 1)
            attrs = ''
            if ':' in name:
                attrs, name = name.split(':', 1)
                commands.append(shlex.join(['declare', f'-{attrs}', f'{name}={value}']))
            else:
                commands.append(shlex.join(['declare', f'{name}={value}']))
        commands.append(shlex.join([args.command, *args.args]))

        # actually run the subprocess; we -tried- using subprocess.run() and ran
        # into quirkiness as to whether or not it finished during SIGINT, but
        # that may have been a race condition we fixed later... we're just
        # leaving it like this because we don't need any features of run() and
        # this works (so we're... not touching it, lol)
        process = subprocess.Popen(
            ['/usr/bin/env', 'bash', '-c', ' && '.join(commands)],
            stdin = handles[0],
            stdout = handles[1],
            stderr = handles[2],
        )
        status = process.wait()

################################################################################

diff = 0
changed = False

def run_timer(output_cb):
    global diff, changed

    start = time.time()
    last_elapsed = 0
    # if we were given an initial delay, wait either until that delay elapses or
    # the runner has released our signaling lock
    if timer_lock.acquire(timeout=args.delay):
        return
    # then, use the signaling lock to handle the interval between output
    while True:
        diff = time.time() - start
        elapsed = int(diff)
        changed = elapsed != last_elapsed
        output_cb()
        last_elapsed = elapsed
        if timer_lock.acquire(timeout=args.interval):
            break
    timer_lock.release()

blank_nums = str.maketrans('0123456789', '----------')

def make_fmt_args(initial=False):
    fmt_args = {
        'i': ' ' * args.indent,
        'min': int(diff) // 60,
        'sec': int(diff) % 60,
        'F': F,
    }
    fmt_args['raw_msg'] = args.message.format(**fmt_args)
    fmt_args['msg'] = args.format_message.format(**fmt_args)
    fmt_args['timer'] = args.format_timer.format(**fmt_args)
    if status is None:
        fmt_args['status'] = args.status_running.format(**fmt_args)
    elif status == 0:
        fmt_args['status'] = args.status_success.format(**fmt_args)
    else:
        fmt_args['status'] = args.status_failure.format(**fmt_args)
    if (initial or status is not None) and int(diff) == 0:
        fmt_args['timer'] = args.format_zero_timer.format(**fmt_args)
    return fmt_args

def do_plain_timer():
    def plain_output():
        if changed:
            write('.')

    write('{msg} ', msg=args.message)
    run_timer(plain_output)
    if status == 0:
        write('done\n')
    else:
        write('fail\n')

def do_fancy_timer():
    def fancy_output():
        # first, restore our saved cursor position on the screen, save
        # the cursor position again (not sure if we need to do this
        # every time, but I'm also not sure how cross-emulator that
        # behavior is, so we do it just to be safe), then clear the line
        write(RESTORE_POS, flush=False)
        write(SAVE_POS, flush=False)
        write(args.format_standard, flush=False, **make_fmt_args())
        write(CLEAR_LINE)

    try:
        if args.hide_cursor:
            write(HIDE_CURSOR, flush=False)
        write(SAVE_POS, flush=False)
        write(CLEAR_LINE, flush=False)
        if args.delay:
            write(args.format_initial, **make_fmt_args(initial=True))
        run_timer(fancy_output)
        needs_final = args.leave or (args.leave_on_failure and status != 0)
        write(RESTORE_POS, flush=False)
        write(CLEAR_LINE, flush=not needs_final)
        if needs_final:
            write(args.format_final, **make_fmt_args())
    finally:
        if args.hide_cursor:
            write(SHOW_CURSOR)

################################################################################

def write(fmt, file=sys.stderr, flush=True, **kw):
    line = fmt.format(**kw) if kw else fmt
    file.write(line)
    if flush:
        file.flush()

def handle_signal(*a):
    return True

################################################################################

signal.signal(signal.SIGINT, handle_signal)
signal.signal(signal.SIGTERM, handle_signal)

if sys.stderr.isatty() and not args.plain:
    do_timer = do_fancy_timer
else:
    do_timer = do_plain_timer

process_thread = threading.Thread(target=do_command)
with sync_cond:
    process_thread.start()
    sync_cond.wait()
timer_thread = threading.Thread(target=do_timer)
timer_thread.start()

process_thread.join()
timer_thread.join()

if status is None:
    # (for lack of a better code, use the same one `wait' returns when the
    # child PID is invalid... it's "sort of sensical" and also not-success)
    sys.exit(127)
elif status < 0:
    sys.exit(128 - status)
else:
    sys.exit(status)

