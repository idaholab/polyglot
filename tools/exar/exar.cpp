/* This file is part of Polyglot.
 
  Copyright (C) 2024, Battelle Energy Alliance, LLC ALL RIGHTS RESERVED

  Polyglot is free software; you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation; either version 3, or (at your option) any later version.

  Polyglot is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along with
  this software; if not see <http://www.gnu.org/licenses/>. */

#include <iostream>
#include <utility>
#include <functional>
#include <fstream>
#include <ranges>
#include <cstring>
#include <memory>
#include <sstream>
#include <set>
#include <map>
#include <vector>
#include <filesystem>
#include <cstdarg>

#define VERSION "0.1"

#if __has_include(<spanstream>)
#define use_spanstream 1
#include <spanstream>
#else
#define use_spanstream 0
#include <strstream>
#endif

#include <getopt.h>

#include "endian.hpp"
#include "ar.hpp"

namespace fs = std::filesystem;



using shared_istream = std::shared_ptr<std::istream>;

class Archive;
using shared_archive = std::shared_ptr<Archive>;


auto make_memory_stream(char* buf, size_t size)
{
#if use_spanstream
    // this is the clean way to do this for C++23
    return std::make_shared<std::spanstream>(
        std::span { buf, (size_t)size }
    );
#else
    // ...but libc++ (clang) sucks and lacks spanstream? really?
    auto ss = std::make_shared<std::strstream>(
        buf, (int)size, std::ios::in | std::ios::binary
    );
    /* XXX: ...why?! If we don't do this, we'll always fail to parse, which
     *      makes...  no sense at all. It seems like it fails to understand the
     *      buffer we're working with or something? (seeking to 5, then to 0
     *      works, but seeking to 1, then to 0 doesn't...) This pretty clearly
     *      seems like a bug, but frankly... this is the deprecated, -wrong- to
     *      do this, but we don't have spanstream on macOS/Clang, soooo... */
    ss->seekg(0, std::ios::end);
    ss->seekg(0);
    return ss;
#endif
}


constexpr size_t align(size_t value, size_t alignment)
{
    return value + (value % alignment);
}


template <class T>
struct format_arg
{
    static auto get(const T& value)
    {
        return value;
    }

    static auto get(T&& value)
    {
        return std::forward<T>(value);
    }
};

template <>
struct format_arg<std::string>
{
    static auto get(const std::string& value)
    {
        return value.c_str();
    }
};

template <>
struct format_arg<std::string_view>
{
    static auto get(std::string_view value)
    {
        return value.data();
    }
};

template <size_t N, class... Args>
void __attribute__((format(printf, 2, 3)))
    format_field(char (&field)[N], const char* format, ...)
{
    va_list ap;
    char buf[N+1];
    memset(buf, 0, sizeof(buf));
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    memcpy(field, buf, std::min(strnlen(buf, sizeof(buf)), N));
}


template <class T, char End, size_t Base, size_t N>
struct field_parser;

template <char End, size_t Base, size_t N>
struct field_parser<std::string_view, End, Base, N>
{
    static std::string_view parse(const char (&buf)[N])
    {
        constexpr auto npos = std::string_view::npos;
        if constexpr (End == 0) {
            return { buf, strnlen(buf, N) };
        } else {
            std::string_view sv = { buf, N };
            auto p = sv.find(End);
            return { p == npos ? sv : sv.substr(0, p) };
        }
    }
};

template <char End, size_t Base, size_t N>
struct field_parser<std::string, End, Base, N>
{
    static std::string parse(const char (&buf)[N])
    {
        return std::string { field_parser<std::string_view, End, Base, N>::parse(buf) };
    }
};

template <std::signed_integral S, char End, size_t Base, size_t N>
struct field_parser<S, End, Base, N>
{
    static S parse(const char (&buf)[N])
    {
        auto sv = field_parser<std::string, End, Base, N>::parse(buf);
        return static_cast<S>(std::stoll(sv, nullptr, Base));
    }
};

template <std::unsigned_integral U, char End, size_t Base, size_t N>
struct field_parser<U, End, Base, N>
{
    static U parse(const char (&buf)[N])
    {
        auto sv = field_parser<std::string, End, Base, N>::parse(buf);
        return static_cast<U>(std::stoull(sv, nullptr, Base));
    }
};

template <class T = std::string_view, char End = 0, size_t Base = 10, size_t N>
auto parse_field(const char (&buf)[N])
{
    return field_parser<T, End, Base, N>::parse(buf);
}

template <char End = 0, size_t Base = 10, class T = std::string_view, size_t N>
void parse_field_into(T* field, const char (&buf)[N])
{
    *field = field_parser<T, End, Base, N>::parse(buf);
}

template <size_t N>
void write_stringstream_into_field(char (&field)[N], std::stringstream ss)
{
    auto s = ss.str();
    memcpy(field, s.data(), std::min(N, s.size()));
}

struct entry
{
    std::string_view path;
    shared_istream stream;
    size_t header_offset;
    size_t content_offset;
    size_t content_size;

    std::string name;
    unsigned long date;
    unsigned uid;
    unsigned gid;
    unsigned mode;

    void copy_content_to(std::ostream& os, size_t alignment = 1) const
    {
        char buf[10240];
        size_t remain = content_size, len;

        stream->seekg(content_offset);

        while (remain) {
            stream->read(buf, std::min(remain, sizeof(buf)));
            if (!(len = stream->gcount()))
                break;
            os.write(buf, len);
            remain -= len;
        }
        if (remain) {
            memset(buf, 0, sizeof(buf));
            while (remain) {
                os.write(buf, std::min(remain, sizeof(buf)));
                remain -= len;
            }
        }
        if ((len = content_size % alignment)) {
            memset(buf, 0, len);
            os.write(buf, len);
        }
    }
};

const std::set<std::string_view> format_files {
    "__.SYMDEF",
    "__.SYMDEF SORTED",
    "/",
};

class Archive
{
protected:
    shared_istream stream;
    std::vector<entry> headers;

    template <std::integral I>
    bool check_magic(I magic)
    {
        constexpr auto size = sizeof(I);
        union {
            I value;
            char data[size];
        } u;

        stream->seekg(0);
        stream->read(u.data, size);

        return (swap_endian<endian::little>(u.value) == magic)
            || (swap_endian<endian::big   >(u.value) == magic)
            || (swap_endian<endian::mixed >(u.value) == magic);
    }

    bool check_magic(std::string_view magic)
    {
        auto size = magic.size();
        char data[size];
        std::string_view value { data, size };

        stream->seekg(0);
        stream->read(data, size);

        return value == magic;
    }

public:
    Archive(shared_istream stream)
        : stream { stream }
        , headers {}
    {}

    virtual std::string description() const
    {
        return "unknown archive format";
    }

    auto get_members() const
    {
        return headers | std::views::filter([](auto& e) {
            return format_files.find(e.name) == format_files.end();
        });
    }
};


namespace common {

template <class T> concept has_name = requires (T&& h) { h.ar_name; };
template <class T> concept has_date = requires (T&& h) { h.ar_date; };
template <class T> concept has_uid  = requires (T&& h) { h.ar_uid;  };
template <class T> concept has_gid  = requires (T&& h) { h.ar_gid;  };
template <class T> concept has_mode = requires (T&& h) { h.ar_mode; };
template <class T> concept has_size = requires (T&& h) { h.ar_size; };

template <auto Magic,
          class Header,
          size_t Alignment,
          endian Endian>
class Archive
    : public ::Archive
{
public:
    using header_type = Header;
    static constexpr auto alignment = Alignment;
    static constexpr auto endianness = Endian;
    static constexpr auto magic = Magic;

    Archive(shared_istream stream)
        : ::Archive { stream }
    {
        if (!check_magic(magic))
            throw std::exception {};
        read_headers();
    }

    virtual std::string description() const
    {
        std::stringstream ss;
        ss << "unknown archive format, " << endianness;
        return ss.str();
    }

protected:
    void read_header(entry& ent)
    {
        header_type hdr;
        // prepopulate fields we already know about
        ent.stream = stream;
        ent.header_offset = stream->tellg();
        ent.content_offset = ent.header_offset + sizeof(header_type);
        // read header structure from the stream
        stream->read((char*)&hdr, sizeof(hdr));
        // name and content_size are required
        ent.name = parse_field<std::string>(hdr.ar_name);
        ent.content_size = swap_endian<endianness>(hdr.ar_size);
        // any others may or may not appear
        if constexpr (has_date<header_type>)
            ent.date = swap_endian<endianness>(hdr.ar_date);
        if constexpr (has_uid<header_type>)
            ent.uid = swap_endian<endianness>(hdr.ar_uid);
        if constexpr (has_gid<header_type>)
            ent.gid = swap_endian<endianness>(hdr.ar_gid);
        if constexpr (has_mode<header_type>)
            ent.mode = swap_endian<endianness>(hdr.ar_mode);
    }

    void read_headers()
    {
        entry ent;
        size_t pos = sizeof(magic);
        size_t end = stream->seekg(0, std::ios::end).tellg();
        while (pos <= (end - sizeof(header_type))) {
            stream->seekg(pos);
            read_header(ent);
            if (!ent.name.size() || (ent.name.at(0) == 0))
                throw std::exception {};
            headers.push_back(ent);
            pos = ent.content_offset + align(ent.content_size, alignment);
        }
        if (pos != end)
            throw std::exception {};
    }

    static void write_entry(const entry& ent, std::ostream& stream)
    {
        header_type hdr;
        // clear out header so we don't have any "bonus" data
        memset((char*)&hdr, 0, sizeof(hdr));
        // if our name is too large, then just truncate with a dumb hash
        if (ent.name.size() > sizeof(hdr.ar_name)) {
            unsigned short sum = 0;
            for (size_t i = 0; i < ent.name.size(); ++i)
                sum += ent.name.at(i);
            sum += ent.name.size();
            format_field(hdr.ar_name, "%.*s%04hx", (int)sizeof(hdr.ar_name)-4,
                         ent.name.c_str(), sum);
        } else {
            format_field(hdr.ar_name, "%s", ent.name.c_str());
        }
        // save the size for free too
        hdr.ar_size = swap_endian_to<endianness>(ent.content_size, hdr.ar_size);
        // now store any of the other fields we have
        if constexpr (has_date<header_type>)
            hdr.ar_date = swap_endian_to<endianness>(ent.date, hdr.ar_date);
        if constexpr (has_uid<header_type>)
            hdr.ar_uid = swap_endian_to<endianness>(ent.uid, hdr.ar_uid);
        if constexpr (has_gid<header_type>)
            hdr.ar_gid = swap_endian_to<endianness>(ent.gid, hdr.ar_gid);
        if constexpr (has_mode<header_type>)
            hdr.ar_mode = swap_endian_to<endianness>(ent.mode, hdr.ar_mode);
        // and write the header and content out to the stream
        stream.write((char*)&hdr, sizeof(hdr));
        ent.copy_content_to(stream, alignment);
    }

public:
    static void write(std::ostream& os, shared_archive archive)
    {
        auto m = swap_endian<endian::native, Endian>(magic);
        os.seekp(0);
        os.write((char*)&m, sizeof(m));
        for (auto& entry : archive->get_members()) {
            write_entry(entry, os);
        }
    }
};

template <template<endian> class A>
std::shared_ptr<::Archive> detect(shared_istream is)
{
    try {
        return std::make_shared<A<endian::little>>(is);
    } catch (std::exception&) {}
    try {
        return std::make_shared<A<endian::big>>(is);
    } catch (std::exception&) {}
    try {
        return std::make_shared<A<endian::mixed>>(is);
    } catch (std::exception&) {}
    throw std::exception {};
}

namespace ancient {

constexpr size_t alignment = 2;

template <endian Endian>
class Archive
    : public common::Archive<magic, ar_hdr, alignment, Endian>
{
public:
    Archive(shared_istream is)
        : common::Archive<magic, ar_hdr, alignment, Endian> { is }
    {}

    virtual std::string description() const
    {
        std::stringstream ss;
        ss << "ancient UNIX 16-bit archive format, " << this->endianness;
        return ss.str();
    }
};

std::shared_ptr<::Archive> detect(shared_istream is)
{
    return common::detect<Archive>(is);
}

} // ::ancient

namespace old {

constexpr size_t alignment = 2;

template <endian Endian>
class Archive
    : public common::Archive<magic, ar_hdr, alignment, Endian>
{
public:
    Archive(shared_istream is)
        : common::Archive<magic, ar_hdr, alignment, Endian> { is }
    {}

    virtual std::string description() const
    {
        std::stringstream ss;
        ss << "old UNIX 16-bit archive format, " << this->endianness;
        return ss.str();
    }
};


std::shared_ptr<::Archive> detect(shared_istream is)
{
    return common::detect<Archive>(is);
}

} // ::old

namespace current {

constexpr auto alignment = 2;

class Archive
    : public ::Archive
{
public:
    Archive(shared_istream stream)
        : ::Archive { stream }
    {
        if (!check_magic(magic))
            throw std::exception {};
        read_headers();
    }

    virtual std::string description() const
    {
        return "current format archive";
    }

protected:
    void read_header(entry& ent)
    {
        ar_hdr hdr;
        // prepopulate fields we already know about
        ent.stream = stream;
        ent.header_offset = stream->tellg();
        ent.content_offset = ent.header_offset + sizeof(ar_hdr);
        // read header data from the stream
        stream->read((char*)&hdr, sizeof(hdr));
        // make sure the file header magic matches
        if (parse_field(hdr.ar_fmag) != fmag)
            throw std::exception {};
        // set up these fields first so we can process extended names
        parse_field_into<' '>(&ent.content_size, hdr.ar_size);
        // extract the name field from the struct
        auto name = parse_field<std::string_view, ' '>(hdr.ar_name);
        // if it's an extended name, find it, then move the boundaries around
        if (name.starts_with(extended)) {
            std::string lenstr { name.substr(extended.size()) };
            auto namelen = std::stoull(lenstr);
            char buf[namelen];
            stream->read(buf, namelen);
            ent.name = std::string { buf, strnlen(buf, namelen) };
            ent.content_size -= namelen;
            ent.content_offset += namelen;
        // otherwise, our name is just our name
        } else {
            ent.name = std::string { name };
        }
        // parse the remaining fields out
        parse_field_into<' '>(&ent.date, hdr.ar_date);
        parse_field_into<' '>(&ent.uid, hdr.ar_uid);
        parse_field_into<' '>(&ent.gid, hdr.ar_gid);
        parse_field_into<' ', 8>(&ent.mode, hdr.ar_mode);
    }

    void read_headers()
    {
        entry ent;
        size_t pos = magic.size();
        size_t end = stream->seekg(0, std::ios::end).tellg();
        while (pos <= (end - sizeof(ar_hdr))) {
            stream->seekg(pos);
            read_header(ent);
            if (!ent.name.size() || (ent.name.at(0) == 0))
                throw std::exception {};
            headers.push_back(ent);
            pos = ent.content_offset + align(ent.content_size, alignment);
        }
    }

    static void write_entry(const entry& ent, std::ostream& stream)
    {
        constexpr auto npos = std::string::npos;
        std::stringstream ss, extra;
        ar_hdr hdr;

        memset(&hdr, ' ', sizeof(hdr));

        if ((ent.name.find(' ') != npos)
                || (ent.name.size() > sizeof(hdr.ar_name))) {
            char buf[align(ent.name.size() + 1, 16)];
            format_field(hdr.ar_name, "%s%lu", extended.data(), sizeof(buf));
            memset(buf, 0, sizeof(buf));
            snprintf(buf, sizeof(buf), "%s", ent.name.c_str());
            extra.write(buf, sizeof(buf));
        } else {
            format_field(hdr.ar_name, "%s", ent.name.c_str());
        }
        auto e = extra.str();

        format_field(hdr.ar_date, "%lu", ent.date);
        format_field(hdr.ar_uid, "%u", ent.uid);
        format_field(hdr.ar_gid, "%u", ent.gid);
        format_field(hdr.ar_mode, "%o", ent.mode);
        format_field(hdr.ar_size, "%lu", ent.content_size + e.size());
        format_field(hdr.ar_fmag, "%s", fmag.data());

        stream.write((char*)&hdr, sizeof(hdr));
        stream.write(e.data(), e.size());
        ent.copy_content_to(stream, alignment);
    }

public:
    static void write(std::ostream& os, shared_archive archive)
    {
        os.seekp(0);
        os.write(magic.data(), magic.size());
        for (auto& entry : archive->get_members()) {
            write_entry(entry, os);
        }
    }
};

std::shared_ptr<::Archive> detect(shared_istream is)
{
    return std::make_shared<Archive>(is);
}

} // ::current

} // ::common

namespace bsd {

namespace old3 {

constexpr size_t alignment = 2;

template <endian Endian>
class Archive
    : public common::Archive<magic, ar_hdr, alignment, Endian>
{
public:
    Archive(shared_istream is)
        : common::Archive<magic, ar_hdr, alignment, Endian> { is }
    {}

    virtual std::string description() const
    {
        std::stringstream ss;
        ss << "old BSD 32-bit archive format, " << this->endianness;
        return ss.str();
    }
};

std::shared_ptr<::Archive> detect(shared_istream is)
{
    return common::detect<Archive>(is);
}

} // ::old3

} // ::bsd


using detector = std::function<shared_archive(shared_istream)>;
using constructor = std::function<void(std::ostream&, shared_archive)>;

static std::map<std::string_view, std::pair<detector, constructor>> formats = {
    { "current",          { common::current::detect,
                            common::current::Archive::write } },
    { "old",              { common::old::detect,
                            common::old::Archive<endian::native>::write } },
    { "old:little",       { common::old::detect,
                            common::old::Archive<endian::little>::write } },
    { "old:big",          { common::old::detect,
                            common::old::Archive<endian::big>::write } },
    { "old:mixed",        { common::old::detect,
                            common::old::Archive<endian::mixed>::write } },
    { "ancient",          { common::ancient::detect,
                            common::ancient::Archive<endian::native>::write } },
    { "ancient:little",   { common::ancient::detect,
                            common::ancient::Archive<endian::little>::write } },
    { "ancient:big",      { common::ancient::detect,
                            common::ancient::Archive<endian::big>::write } },
    { "ancient:mixed",    { common::ancient::detect,
                            common::ancient::Archive<endian::mixed>::write } },
    { "bsd:old",          { bsd::old3::detect,
                            bsd::old3::Archive<endian::native>::write } },
    { "bsd:old:little",   { bsd::old3::detect,
                            bsd::old3::Archive<endian::little>::write } },
    { "bsd:old:big",      { bsd::old3::detect,
                            bsd::old3::Archive<endian::big>::write } },
    { "bsd:old:mixed",    { bsd::old3::detect,
                            bsd::old3::Archive<endian::mixed>::write } },
};

shared_archive detect_any_format(shared_istream is)
{
    auto functions = formats | std::views::transform(
        [](auto& e) { return e.second.first; }
    );
    for (auto e : functions) {
        try {
            return e(is);
        } catch (std::exception&) {}
    }
    throw std::exception {};
}



std::string_view prog;


static constexpr auto arcv_opts = "hv";
static constexpr option arcv_longopts[] = {
    { "help",       0, nullptr, 'h' },
    { "version",    0, nullptr, 'v' },
    { NULL },
};

void arcv_usage(std::ostream& os)
{
    os << "Usage: " << prog << " [-h/-v] <archive>..." << std::endl;
}

void arcv_version(std::ostream& os)
{
    os << "exar version " << VERSION << std::endl;
}

void arcv_help(std::ostream& os)
{
    arcv_usage(os);
    os << std::endl
       << "Positional arguments:" << std::endl
       << "  <archive>     archive to convert to modern format" << std::endl
       << "Optional arguments:" << std::endl
       << "  -h/--help     print this help message" << std::endl
       << "  -v/--version  print program version information" << std::endl
       << std::endl;
}

int arcv_main(int argc, char **argv)
{
    int opt;

    while ((opt = getopt_long(argc, argv, arcv_opts, arcv_longopts, nullptr)) >= 0) {
        switch (opt)
        {
        case 'h':
            arcv_help(std::cout);
            return EXIT_SUCCESS;
        case 'v':
            arcv_version(std::cout);
            return EXIT_SUCCESS;
        case '?':
        default:
            arcv_usage(std::cerr);
            return EXIT_FAILURE;
        }
    }

    // process each path given on our command line
    for (size_t i = optind; i < argc; ++i) {
        fs::path path { argv[i] };
        switch (fs::status(path).type())
        {
        // if it's a regular file, try to process it
        case fs::file_type::regular:
            {
                // open our file, figure out the buffer size, then allocate
                std::fstream f { path, std::ios::in };
                f.seekg(0, std::ios::end);
                size_t size = f.tellg();
                auto buf = new char[size];
                // actually try to process our input archive
                try {
                    // read the file contents into our buffer
                    f.seekg(0);
                    f.read(buf, size);
                    if (f.gcount() != size)
                        std::cerr << "While processing "
                                  << path << ", expected " << size
                                  << " bytes but read " << f.gcount()
                                  << "; continuing." << std::endl;
                    size = f.gcount();
                    f.close();
                    // build a memory stream, then parse the archive
                    auto ss = make_memory_stream(buf, size);
                    auto arc = detect_any_format(ss);
                    // reopen the file, truncating it, then write the archive
                    f.open(path, std::ios::out | std::ios::trunc);
                    common::current::Archive::write(f, arc);
                    std::cout << "Converted " << path << std::endl;

                } catch (std::exception& exc) {
                    std::cerr << "While processing " << path
                              << ", encountered an error: " << exc.what()
                              << std::endl;
                }
                // clean up our buffer
                delete[] buf;
            }
            break;

        // it was... something else, so log an error and continue
        case fs::file_type::not_found:
            std::cerr << "Skipping " << path << ": does not exist" << std::endl;
            break;
        case fs::file_type::none:
        case fs::file_type::unknown:
        default:
            std::cerr << "Skipping " << path << ": not a regular file"
                      << std::endl;
            break;
        }
    }
    return EXIT_SUCCESS;
}

static constexpr auto opts = "hvCIxtci:f:";
static constexpr option longopts[] = {
    { "help",           0, nullptr, 'h' },
    { "version",        0, nullptr, 'v' },
    { "identify",       0, nullptr, 'I' },
    { "list",           0, nullptr, 't' },
    { "convert",        0, nullptr, 'C' },
    { "create",         0, nullptr, 'c' },
    { "extract",        0, nullptr, 'x' },
    { "input-format",   1, nullptr, 'i' },
    { "output-format",  1, nullptr, 'f' },
    { NULL },
};

enum class run_action
{
    none,
    convert,
    identify,
    extract,
    list,
    create,
};

void usage(std::ostream& os)
{
    os << "Usage: " << prog << " [-h/-v] (-I/-C/-t/-c/-x) [-i<fmt>] [-f<fmt>]"
                               " <archive> [...]" << std::endl;
}

void version(std::ostream& os)
{
    os << "exar version " << VERSION << std::endl;
}

void help(std::ostream& os)
{
    os << "Usage: " << prog << " (-h/--help/-v/--version)" << std::endl
       << "       " << prog << " (-I/--identify) <archive>" << std::endl
       << "       " << prog << " (-t/--list) [-i<fmt>] <archive>"
                            << std::endl
       << "       " << prog << " (-C/--convert) [-i<fmt>] [-f<fmt>] <archive>"
                               " <output>" << std::endl
       << "       " << prog << " (-c/--create) [-f<fmt>] <output> <path>..."
                            << std::endl
       << "       " << prog << " (-x/--extract) [-i<fmt>] <archive> [<path>...]"
                            << std::endl
       << std::endl
       << "Optional arguments:" << std::endl
       << "  -h/--help       print this help message" << std::endl
       << "  -v/--version    print program version information" << std::endl
       << std::endl
       << "Actions:" << std::endl
       << "  -I/--identify   detect archive format" << std::endl
       << "  -t/--list       list archive contents" << std::endl
       << "  -C/--convert    convert existing archive to another format"
                             << std::endl
       << "  -c/--create     create an archive from a set of files" << std::endl
       << "  -x/--extract    extract files from an archive" << std::endl
       << std::endl
       << "Positional arguments:" << std::endl
       << "  <archive>       archive to operate on" << std::endl
       << "  <output>        output archive to create" << std::endl
       << "  <path>          [create] paths to files to add to archive"
                             << std::endl
       << "                  [extract] paths within the archive to extract"
                             << std::endl
       << std::endl
       << "Optional arguments:" << std::endl
       << "  -i<fmt>/--input-format <fmt>" << std::endl
       << "                  format of input archive, or '?' to list formats"
                             << std::endl
       << "  -f<fmt>/--output-format <fmt>" << std::endl
       << "                  format of output archive, or '?' to list formats"
                             << std::endl
       << std::endl;

}

int main(int argc, char **argv)
{
    prog = argv[0];

    // if we were called as 'arcv' (in any form), just do that instead
    if ((prog == "arcv") || prog.ends_with("-arcv") || prog.ends_with("/arcv"))
        return arcv_main(argc, argv);

    int opt;
    run_action action = run_action::none;
    std::string input;
    std::vector<std::string> operands;
    detector detect = detect_any_format;
    constructor construct = common::current::Archive::write;

    while ((opt = getopt_long(argc, argv, opts, longopts, nullptr)) >= 0) {
        switch (opt)
        {
        case 'C': action = run_action::convert;  break;
        case 'I': action = run_action::identify; break;
        case 'x': action = run_action::extract;  break;
        case 't': action = run_action::list;    break;
        case 'c': action = run_action::create;   break;

        case 'i':
        case 'f':
            {
                std::string_view o { optarg };
                if (o == "?") {
                    std::cout << "valid formats:";
                    for (auto& e : formats)
                        std::cout << " " << e.first;
                    std::cout << std::endl;
                    return EXIT_SUCCESS;
                }
                auto f = formats.find(o);
                if (f == formats.end()) {
                    std::cerr << "invalid format: '" << o << "'" << std::endl;
                    return EXIT_FAILURE;
                } else {
                    switch (opt)
                    {
                    case 'i':
                        detect = f->second.first;
                        break;
                    case 'f':
                        construct = f->second.second;
                        break;
                    }
                }
            }
            break;

        case 'h':
            help(std::cout);
            return EXIT_SUCCESS;
        case 'v':
            version(std::cout);
            return EXIT_SUCCESS;
        case '?':
        default:
            usage(std::cerr);
            return EXIT_FAILURE;
        }
    }

    if (optind < argc) {
        input = argv[optind++];
    }
    while (optind < argc) {
        operands.emplace_back(argv[optind++]);
    }

    switch (action)
    {
    case run_action::none:
        std::cerr << "No action specified." << std::endl;
        return EXIT_FAILURE;

    case run_action::convert:
        if (operands.size() > 1) {
            std::cerr << "Too many operand files specified." << std::endl;
            return EXIT_FAILURE;
        }
        {
            shared_istream i = std::make_shared<std::ifstream>(input);
            std::ofstream o { operands[0] };
            auto archive = detect(i);
            construct(o, archive);
        }
        return EXIT_SUCCESS;

    case run_action::identify:
        if (operands.size() > 0) {
            std::cerr << "Too many operand files specified." << std::endl;
            return EXIT_FAILURE;
        }
        {
            shared_istream f = std::make_shared<std::ifstream>(input);
            auto archive = detect(f);
            std::cout << archive->description() << std::endl;
        }
        return EXIT_SUCCESS;

    case run_action::list:
        if (operands.size() > 0) {
            std::cerr << "Too many operand files specified." << std::endl;
            return EXIT_FAILURE;
        }
        {
            shared_istream f = std::make_shared<std::ifstream>(input);
            auto archive = detect(f);
            for (auto& e : archive->get_members()) {
                std::cout << e.name << std::endl;
            }
        }
        return EXIT_SUCCESS;

    case run_action::extract:
        std::cerr << "FIXME: archive extraction is currently not implemented!"
                  << std::endl;
        return EXIT_FAILURE;

    case run_action::create:
        if (operands.size() == 0) {
            std::cerr << "Cowardly refusing to create an empty archive."
                      << std::endl;
            return EXIT_FAILURE;
        }
        std::cerr << "FIXME: archive creation is currently not implemented!"
                  << std::endl;
        return EXIT_FAILURE;
    }
}

