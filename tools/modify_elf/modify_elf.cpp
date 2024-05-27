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
#include <elfio/elfio.hpp>
#include <ranges>
#include <map>
#include <getopt.h>

using namespace ELFIO;



struct symbol_info
{
    std::string     name;
    Elf64_Addr      value;
    Elf_Xword       size;
    unsigned char   bind;
    unsigned char   type;
    Elf_Half        section_index;
    unsigned char   other;
};


template <class I, class S, class T>
class index_iterator
{
public:
    index_iterator(T& iterable, S index)
        : iterable(iterable)
        , index(index)
    {}

    bool operator!=(const index_iterator& other) const
    {
        return index != other.index;
    }

    I operator*() const
    {
        return iterable.at(index);
    }

    index_iterator& operator++()
    {
        ++index;
        return *this;
    }

private:
    T& iterable;
    S index;
};


template <class I, class S>
class iterable
{
public:
    template <class T>
    using iter = index_iterator<I, S, T>;
    template <class T>
    using citer = index_iterator<I, S, const T>;

    virtual I at(S) const = 0;
    virtual S size() const = 0;

    auto begin() -> iter<typeof(*this)>
    { return { *this, 0 }; }
    auto cbegin() const -> iter<typeof(*this)>
    { return { *this, 0 }; }
    auto begin() const
    { return cbegin(); }
    auto end() -> iter<typeof(*this)>
    { return { *this, size() }; }
    auto cend() const -> iter<typeof(*this)>
    { return { *this, size() }; }
    auto end() const
    { return cend(); }
};


class better_string_section_accessor
    : public string_section_accessor
{
protected:
    using parent = string_section_accessor;

public:
    better_string_section_accessor(std::unique_ptr<section>& section)
        : string_section_accessor(section.get())
    {}

    better_string_section_accessor(section* section)
        : string_section_accessor(section)
    {}
};


class better_symbol_section_accessor
    : public symbol_section_accessor
    , public iterable<symbol_info, size_t>
{
protected:
    using parent = symbol_section_accessor;
    using iter = iterable<symbol_info, size_t>::iter<better_symbol_section_accessor>;

public:
    better_symbol_section_accessor(const elfio& elf,
                                   std::unique_ptr<section>& section)
        : symbol_section_accessor(elf, section.get())
    {}

    better_symbol_section_accessor(const elfio& elf, section* section)
        : symbol_section_accessor(elf, section)
    {}

    virtual symbol_info at(size_t index) const
    {
        return by_index(index);
    };

    virtual size_t size() const
    {
        return get_symbols_num();
    }

    symbol_info by_index(Elf_Xword index) const
    {
        symbol_info i {};
        auto r = get_symbol(index, i.name, i.value, i.size, i.bind, i.type,
                            i.section_index, i.other);
        if (!r)
            throw std::out_of_range { "symbol with index doesn't exist" };
        return i;
    }

    symbol_info by_name(std::string&& name) const
    {
        return this->by_name(name);
    }

    symbol_info by_name(const std::string& name) const
    {
        symbol_info i { .name = name };
        auto r = get_symbol(name, i.value, i.size, i.bind, i.type,
                            i.section_index, i.other);
        if (!r)
            throw std::out_of_range { "symbol with name doesn't exist" };
        return i;
    }

    symbol_info by_value(Elf64_Addr value) const
    {
        symbol_info i { .value = value };
        auto r = get_symbol(value, i.name, i.size, i.bind, i.type,
                            i.section_index, i.other);
        if (!r)
            throw std::out_of_range { "symbol with value doesn't exist" };
        return i;
    }

    iter add_symbol(Elf_Word name, Elf64_Addr value, Elf_Xword size,
                    unsigned char info, unsigned char other, Elf_Half shndx)
    {
        return { *this, parent::add_symbol(name, value, size, info, other,
                                           shndx) };
    }

    iter add_symbol(Elf_Word name, Elf64_Addr value, Elf_Xword size,
                        unsigned char bind, unsigned char type,
                        unsigned char other, Elf_Half shndx)
    {
        return { *this, parent::add_symbol(name, value, size, bind, type, other,
                                           shndx) };
    }

    iter add_symbol(string_section_accessor& strtab, std::string_view name,
                    Elf64_Addr value, Elf_Xword size, unsigned char info,
                    unsigned char other, Elf_Half shndx)
    {
        return { *this, parent::add_symbol(strtab, name.data(), value, size,
                                           info, other, shndx) };
    }

    iter add_symbol(string_section_accessor& strtab, std::string_view name,
                    Elf64_Addr value, Elf_Xword size, unsigned char bind,
                    unsigned char type, unsigned char other, Elf_Half shndx)
    {
        return { *this, parent::add_symbol(strtab, name.data(), value, size,
                                           bind, type, other, shndx) };
    }

    iter add_symbol(string_section_accessor& strtab, const symbol_info& sym)
    {
        return add_symbol(strtab, sym.name, sym.value, sym.size, sym.bind,
                          sym.type, sym.other, sym.section_index);
    }

};



#include "elf_lookup.hpp"


enum long_option_values
{
    OPT_BASE = 0xff,
    SET_OSABI,
    SET_ABIVERSION,
    SET_BRANDING,
    SET_TYPE,
    SET_MACHINE,
    ADD_SYMBOL,
};

static constexpr auto opts = "h";
static constexpr option longopts[] = {
    { "set-osabi",          1, nullptr, SET_OSABI       },
    { "set-abi-version",    1, nullptr, SET_ABIVERSION  },
    { "set-branding",       1, nullptr, SET_BRANDING    },
    { "set-type",           1, nullptr, SET_TYPE        },
    { "set-machine",        1, nullptr, SET_MACHINE     },
    { "add-symbol",         1, nullptr, ADD_SYMBOL      },
    { NULL },
};


int main(int argc, char **argv)
{
    int opt, longidx;
    bool m;
    size_t v;

    std::vector<std::shared_ptr<action>> actions;
    std::string input, output;

    while ((opt = getopt_long(argc, argv, opts, longopts, &longidx)) >= 0) {
        switch (opt)
        {
        case SET_OSABI:
            actions.emplace_back(set_osabi::parse(optarg));
            break;
        case SET_ABIVERSION:
            actions.emplace_back(set_abiversion::parse(optarg));
            break;
#if 0
        case SET_BRANDING:
            actions.emplace_back(set_branding::parse(optarg));
            break;
#endif
        case SET_TYPE:
            actions.emplace_back(set_type::parse(optarg));
            break;
        case SET_MACHINE:
            actions.emplace_back(set_machine::parse(optarg));
            break;
        case ADD_SYMBOL:
            actions.emplace_back(add_symbol::parse(optarg));
            break;

        case 'h':
            std::cout << "fixme: help" << std::endl;
            return EXIT_SUCCESS;
        case '?':
        default:
            std::cerr << "Usage: " << argv[0] << " ..." << std::endl;
            return EXIT_FAILURE;
        }
    }

    if (argc - optind != 2) {
        std::cerr << "fixme: too few args" << std::endl;
        return EXIT_FAILURE;
    }
    input = argv[optind];
    output = argv[optind+1];

    elfio elf;
    if (!elf.load(input)) {
        std::cout << "unable to load ELF file: " << input << std::endl;
        return EXIT_FAILURE;
    }
    for (auto& action : actions) {
        action->execute(elf);
    }
    elf.save(output);

    return EXIT_SUCCESS;

    /*
    auto&& strings = *(elf.sections | filter_strtab).begin();
    if (strings == nullptr)
        throw std::out_of_range { "could not find string table" };
    auto&& symbols = *(elf.sections | filter_symtab).begin();
    if (symbols == nullptr)
        throw std::out_of_range { "could not find symbol table" };

    better_string_section_accessor strtab { strings };
    better_symbol_section_accessor symtab { elf, symbols };

    symtab.add_symbol(strtab, "hello_world_symbol", 0, 0, STB_GLOBAL, STT_NOTYPE, STV_DEFAULT, SHN_UNDEF);

    elf.save("out.elf");
    */
}


