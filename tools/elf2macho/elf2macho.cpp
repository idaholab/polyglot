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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>
#include <exception>

using namespace std;

const char* progname = "convert";

#if __has_include(<libelf.h>)
#include <libelf.h>
#elif __has_include(<libelf/libelf.h>)
#include <libelf/libelf.h>
#else
#error "Header libelf.h not available!"
#endif

#if __has_include(<gelf.h>)
#include <gelf.h>
#elif __has_include(<libelf/gelf.h>)
#include <libelf/gelf.h>
#else
#error "Header gelf.h not available!"
#endif

// FIXME: these are extensions...
#define EM_AARCH64  183

#include "macho.hpp"


#define progerr cerr << progname << ": "

#define MACH_ALIGN 0x1000
#define ROUND_TO_2POW(m, v) (((v) + (m) - 1) & -(m))
#define CHECK_SEC_NAME(n) \
    elf_check_name(elf, eh.e_shstrndx, esh.sh_name, (n))

#define MODE_X (S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)


enum mach_endianness {
    MACH_MSB,
    MACH_LSB,
};

uint16_t byteswap(uint16_t v)
{
    return (((v >>  8) & 0x00ff)
           |((v <<  8) & 0xff00));
}

int16_t byteswap(int16_t v)
{
    return (int16_t)byteswap((uint16_t)v);
}

uint32_t byteswap(uint32_t v)
{
    return (((v >> 24) & 0x000000ff)
           |((v >>  8) & 0x0000ff00)
           |((v <<  8) & 0x00ff0000)
           |((v << 24) & 0xff000000));
}

int32_t byteswap(int32_t v)
{
    return (int32_t)byteswap((uint32_t)v);
}

uint64_t byteswap(uint64_t v)
{
    return (((v >> 56) & 0x00000000000000ff)
           |((v >> 40) & 0x000000000000ff00)
           |((v >> 24) & 0x0000000000ff0000)
           |((v >>  8) & 0x00000000ff000000)
           |((v <<  8) & 0x000000ff00000000)
           |((v << 24) & 0x0000ff0000000000)
           |((v << 40) & 0x00ff000000000000)
           |((v << 56) & 0xff00000000000000));
}

int64_t byteswap(int64_t v)
{
    return (int64_t)byteswap((uint64_t)v);
}

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define SWAP_LSB
#define SWAP_MSB byteswap
#else
#define SWAP_LSB byteswap
#define SWAP_MSB
#endif

#define make_endian(e, v) \
    ((e) == MACH_MSB ? SWAP_MSB(v) : SWAP_LSB(v))


template <typename T>
void _write(ostream& os, mach_endianness e, T value)
{
    T eulav = make_endian(e, value);
    os.write((char*)&eulav, sizeof(T));
}

void _write(ostream&os, string str, size_t size)
{
    if (str.size() >= size) {
        os.write(str.c_str(), size);
    } else {
        char fill[size - str.size()];
        memset(fill, 0, sizeof(fill));
        os.write(str.c_str(), str.size());
        os.write(fill, sizeof(fill));
    }
}


struct mach_struct
{
    unsigned bits;
    mach_endianness endian;

    virtual uint32_t structsize() = 0;
    virtual ostream& write(ostream& os) = 0;
};

struct mach_cpu
{
    cpu_type_t      type;
    cpu_subtype_t   subtype;
};

#define MACH_CPU(t, s) \
    (mach_cpu){ .type = CPU_TYPE_##t, .subtype = CPU_SUBTYPE_##t##_##s }

struct mach_header : public mach_struct
{
    mach_cpu        cpu;
    uint32_t        filetype;
    uint32_t        ncmds;
    uint32_t        sizeofcmds;
    uint32_t        flags;

    virtual uint32_t structsize()
    {
        return (bits == 32 ? 28 : 32);
    }

    virtual ostream& write(ostream& os)
    {
        if (bits == 32) {
            _write(os, endian, (uint32_t)MH_MAGIC);
        } else {
            _write(os, endian, (uint32_t)MH_MAGIC_64);
        }

        _write(os, endian, cpu.type);
        _write(os, endian, cpu.subtype);
        _write(os, endian, filetype);
        _write(os, endian, ncmds);
        _write(os, endian, sizeofcmds);
        _write(os, endian, flags);

        if (bits == 64) {
            _write(os, endian, (uint32_t)0);
        }
        
        return os;
    }
};

struct load_command : public mach_struct
{
    virtual ~load_command()
    {}

    virtual uint32_t cmdsize() = 0;
};

struct section : public mach_struct
{
    string      sectname;
    string      segname;
    uint64_t    addr;
    uint64_t    size;
    uint32_t    offset;
    uint32_t    align;
    uint32_t    reloff;
    uint32_t    nreloc;
    uint32_t    flags;

    virtual uint32_t structsize()
    {
        return (bits == 32 ? 68 : 80);
    }

    virtual ostream& write(ostream& os)
    {
        if (bits == 32) {
            assert(addr <= 0xffffffff);
            assert(size <= 0xffffffff);
        }

        _write(os, sectname, 16);
        _write(os, segname, 16);

        if (bits == 32) {
            _write(os, endian, (uint32_t)(addr & 0xffffffff));
            _write(os, endian, (uint32_t)(size & 0xffffffff));
        } else {
            _write(os, endian, addr);
            _write(os, endian, size);
        }

        _write(os, endian, offset);
        _write(os, endian, align);
        _write(os, endian, reloff);
        _write(os, endian, nreloc);
        _write(os, endian, flags);
        _write(os, endian, (uint32_t)0);
        _write(os, endian, (uint32_t)0);

        if (bits == 64) {
            _write(os, endian, (uint32_t)0);
        }

        return os;
    }
};

struct segment_command : public load_command
{
    string      segname;        /* segment name */
    uint64_t    vmaddr;         /* memory address of this segment */
    uint64_t    vmsize;         /* memory size of this segment */
    uint64_t    fileoff;        /* file offset of this segment */
    uint64_t    filesize;       /* amount to map from the file */
    vm_prot_t   maxprot;        /* maximum VM protection */
    vm_prot_t   initprot;       /* initial VM protection */
    uint32_t    flags;          /* flags */

    vector<shared_ptr<section>> sections;

    virtual ~segment_command()
    {}

    shared_ptr<section> add_section()
    {
        auto sec = shared_ptr<section>(new section());
        sec->bits = bits;
        sec->endian = endian;
        return sections.emplace_back(sec);
    }

    virtual uint32_t structsize()
    {
        return (bits == 32 ? 56 : 72);
    }

    virtual uint32_t cmdsize()
    {
        uint32_t ts = structsize();
        for (auto s : sections)
            ts += s->structsize();
        return ts;
    }

    virtual ostream& write(ostream& os)
    {
        if (bits == 32) {
            assert(vmaddr <= 0xffffffff);
            assert(vmsize <= 0xffffffff);
            assert(fileoff <= 0xffffffff);
            assert(filesize <= 0xffffffff);
        }

        if (bits == 32) {
            _write(os, endian, (uint32_t)LC_SEGMENT);
        } else {
            _write(os, endian, (uint32_t)LC_SEGMENT_64);
        }

        _write(os, endian, cmdsize());
        _write(os, segname, 16);

        if (bits == 32) {
            _write(os, endian, (uint32_t)(vmaddr & 0xffffffff));
            _write(os, endian, (uint32_t)(vmsize & 0xffffffff));
            _write(os, endian, (uint32_t)(fileoff & 0xffffffff));
            _write(os, endian, (uint32_t)(filesize & 0xffffffff));
        } else {
            _write(os, endian, vmaddr);
            _write(os, endian, vmsize);
            _write(os, endian, fileoff);
            _write(os, endian, filesize);
        }

        _write(os, endian, maxprot);
        _write(os, endian, initprot);
        _write(os, endian, (uint32_t)sections.size());
        _write(os, endian, flags);

        for (auto s : sections) {
            s->segname = segname;
            s->write(os);
        }

        return os;
    }
};


struct thread_state : public mach_struct
{
    virtual uint32_t flavor() = 0;
    virtual uint32_t count() = 0;

    virtual uint32_t structsize()
    {
        return count() * 4;
    }
};


struct unixthread_command : public load_command
{
    shared_ptr<thread_state> state;

    virtual ~unixthread_command()
    {}

    template <typename T>
    shared_ptr<T> add_state()
    {
        state = shared_ptr<thread_state>(new T());
        state->bits = bits;
        state->endian = endian;
        return dynamic_pointer_cast<T>(state);
    }

    virtual uint32_t structsize()
    {
        return 16;
    }

    virtual uint32_t cmdsize()
    {
        assert(state.get() != NULL);
        return structsize() + state->structsize();
    };

    virtual ostream& write(ostream& os)
    {
        assert(state.get() != NULL);
        _write(os, endian, (uint32_t)LC_UNIXTHREAD);
        _write(os, endian, cmdsize());
        _write(os, endian, state->flavor());
        _write(os, endian, state->count());
        state->write(os);
        return os;
    }
};


struct thread_state_x86 : public thread_state
{
    uint32_t eax, ebx, ecx, edx, edi, esi, ebp, esp, eip;
    uint32_t ss, eflags, cs, ds, es, fs, gs;

    virtual uint32_t flavor()
    {
        return x86_THREAD_STATE32;
    }

    virtual uint32_t count()
    {
        return 16;
    }

    virtual ostream& write(ostream& os)
    {
        _write(os, endian, eax);
        _write(os, endian, ebx);
        _write(os, endian, ecx);
        _write(os, endian, edx);
        _write(os, endian, edi);
        _write(os, endian, esi);
        _write(os, endian, ebp);
        _write(os, endian, esp);
        _write(os, endian, ss);
        _write(os, endian, eflags);
        _write(os, endian, eip);
        _write(os, endian, cs);
        _write(os, endian, ds);
        _write(os, endian, es);
        _write(os, endian, fs);
        _write(os, endian, gs);
        return os;
    }
};

struct thread_state_x86_64 : public thread_state
{
    uint64_t rax, rbx, rcx, rdx, rdi, rsi, rbp, rsp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rip, rflags, cs, fs, gs;

    virtual uint32_t flavor()
    {
        return x86_THREAD_STATE64;
    }

    virtual uint32_t count()
    {
        return 42;
    }

    virtual ostream& write(ostream& os)
    {
        _write(os, endian, rax);
        _write(os, endian, rbx);
        _write(os, endian, rcx);
        _write(os, endian, rdx);
        _write(os, endian, rdi);
        _write(os, endian, rsi);
        _write(os, endian, rbp);
        _write(os, endian, rsp);
        _write(os, endian, r8);
        _write(os, endian, r9);
        _write(os, endian, r10);
        _write(os, endian, r11);
        _write(os, endian, r12);
        _write(os, endian, r13);
        _write(os, endian, r14);
        _write(os, endian, r15);
        _write(os, endian, rip);
        _write(os, endian, rflags);
        _write(os, endian, cs);
        _write(os, endian, fs);
        _write(os, endian, gs);
        return os;
    }
};

struct thread_state_powerpc : public thread_state
{
    uint32_t srr0; /* program counter */
    uint32_t srr1;
    uint32_t gpr[32];
    uint32_t cr;
    uint32_t xer;
    uint32_t lr;
    uint32_t ctr;
    uint32_t mq;
    uint32_t vrsave;

    virtual uint32_t flavor()
    {
        return PPC_THREAD_STATE;
    }

    virtual uint32_t count()
    {
        return 40;
    }

    virtual ostream& write(ostream& os)
    {
        _write(os, endian, srr0);
        _write(os, endian, srr1);
        for (int i = 0; i < 32; ++i) {
            _write(os, endian, gpr[i]);
        }
        _write(os, endian, cr);
        _write(os, endian, xer);
        _write(os, endian, lr);
        _write(os, endian, ctr);
        _write(os, endian, mq);
        _write(os, endian, vrsave);
        return os;
    }
};



struct mach_file_segment
{
    uint64_t offset;
    uint64_t size;
    shared_ptr<char> _content;

    mach_file_segment(uint64_t offset, uint64_t segment_size,
            const char *content, size_t content_size)
        : offset(offset), size(segment_size)
    {
        _content = shared_ptr<char>(new char[size]);
        bzero(_content.get(), size);
        if (content && content_size)
            memcpy(_content.get(), content, content_size);
    }

    char *content()
    {
        return _content.get();
    }

    ostream& write(ostream& os)
    {
        return os.write(_content.get(), size);
    }
};

struct mach
{
    mach_endianness endian;
    unsigned bits;
    mach_header header;
    vector<shared_ptr<load_command>> load_commands;
    vector<mach_file_segment> file_segments;

    mach(mach_endianness endian, unsigned bits)
        : endian(endian), bits(bits)
    {
        header.endian = endian;
        header.bits = bits;
    }

    template <typename T>
    shared_ptr<T> add_load_command()
    {
        T *command = new T();
        command->endian = endian;
        command->bits = bits;
        return dynamic_pointer_cast<T>(load_commands.emplace_back(command));
    }

    void add_file_segment(mach_file_segment segment)
    {
        file_segments.push_back(segment);
    }

    uint32_t size_of_commands()
    {
        uint32_t soc = 0;
        for (auto c : load_commands)
            soc += c->cmdsize();
        return soc;
    }

    uint32_t size_of_headers()
    {
        return header.structsize() + size_of_commands();
    }

    ostream& write(ostream& os)
    {
        assert(load_commands.size() > 0);
        assert(file_segments.size() > 0);

        streampos startpos = os.tellp();

        header.ncmds = load_commands.size();
        header.sizeofcmds = size_of_commands();

        header.write(os);
        for (auto c : load_commands)
            c->write(os);

        for (auto s : file_segments) {
            assert(s.offset == (os.tellp() - startpos));
            s.write(os);
        }

        assert(os.tellp() >= 4096);

        return os;
    }
};


struct fat_header
{
    vector<shared_ptr<mach>> arches;
};



template <typename... Params>
void build_string(stringstream& ss, Params... params);

template <typename T, typename... Params>
void build_string(stringstream& ss, T first, Params... rest)
{
    ss << first;
    build_string(ss, rest...);
};

template <>
void build_string(stringstream& ss)
{
    return;
}


class convert_exception : public std::exception
{
protected:
    string message;

public:
    convert_exception()
        : message("<no message>")
    {}

    template <typename... Params>
    convert_exception(Params... params)
        : message()
    {
        stringstream ss;
        build_string(ss, params...);
        message = ss.str();
    }

    const char *what() const noexcept
    {
        return message.c_str();
    }
};





int elf_check_name(Elf *elf, int shstrtab_index, GElf_Word elf_name, const char *want)
{
    const char *name = elf_strptr(elf, shstrtab_index, elf_name);
    if (name == NULL)
        return 0;
    return !strcmp(name, want);
}

shared_ptr<mach> convert(Elf *elf)
{
    GElf_Ehdr eh;
    if (gelf_getehdr(elf, &eh) == NULL)
        throw new convert_exception("unable to get ELF header");
    if (eh.e_type != ET_EXEC)
        throw new convert_exception("ELF is not an executable");

    unsigned bits;
    switch (eh.e_ident[EI_CLASS]) {
    case ELFCLASS32: bits = 32; break;
    case ELFCLASS64: bits = 64; break;
    default:
        throw new convert_exception("ELF has unknown class");
    };

    mach_endianness endian;
    switch (eh.e_ident[EI_DATA]) {
    case ELFDATA2LSB: endian = MACH_LSB; break;
    case ELFDATA2MSB: endian = MACH_MSB; break;
    default:
        throw new convert_exception("ELF has unsupported byte order");
    };

    shared_ptr<mach> m(new mach(endian, bits));
    m->header.filetype = MH_EXECUTE;
    m->header.flags = MH_NOUNDEFS | MH_SPLIT_SEGS;

    switch (eh.e_machine) {
    case EM_VAX:     m->header.cpu = MACH_CPU(VAX,       ALL); break;
    case EM_68K:     m->header.cpu = MACH_CPU(MC680x0,   ALL); break;
    case EM_386:     m->header.cpu = MACH_CPU(X86,       ALL); break;
    case EM_X86_64:  m->header.cpu = MACH_CPU(X86_64,    ALL); break;
    case EM_MIPS:    m->header.cpu = MACH_CPU(MIPS,      ALL); break;
  //case EM_???:     m->header.cpu = MACH_CPU(MC98000,   ALL); break;
    case EM_PARISC:  m->header.cpu = MACH_CPU(HPPA,      ALL); break;
    case EM_ARM:     m->header.cpu = MACH_CPU(ARM,       ALL); break;
    case EM_AARCH64: m->header.cpu = MACH_CPU(ARM64,     ALL); break;
    case EM_88K:     m->header.cpu = MACH_CPU(MC88000,   ALL); break;
    case EM_SPARC:   m->header.cpu = MACH_CPU(SPARC,     ALL); break;
    case EM_860:     m->header.cpu = MACH_CPU(I860,      ALL); break;
    case EM_ALPHA:   m->header.cpu = MACH_CPU(ALPHA,     ALL); break;
    case EM_PPC:     m->header.cpu = MACH_CPU(POWERPC,   ALL); break;
    case EM_PPC64:   m->header.cpu = MACH_CPU(POWERPC64, ALL); break;
    default:
        throw new convert_exception("Unsupported ELF machine");
    }

    auto sg_pgzero  = m->add_load_command<segment_command>();
    auto sg_text    = m->add_load_command<segment_command>();
    auto sc_text    = sg_text->add_section();
    auto sg_rodata  = m->add_load_command<segment_command>();
    auto sc_rodata  = sg_rodata->add_section();
    auto sg_data    = m->add_load_command<segment_command>();
    auto sc_data    = sg_data->add_section();
    auto sc_pinit   = sg_data->add_section();
    auto sc_init    = sg_data->add_section();
    auto sc_fini    = sg_data->add_section();
    auto sg_bss     = m->add_load_command<segment_command>();
    auto sc_bss     = sg_bss->add_section();
    auto th_unix    = m->add_load_command<unixthread_command>();

    sg_pgzero->segname  = "__PAGEZERO";
    sg_pgzero->maxprot  = 0;
    sg_pgzero->initprot = 0;

    sg_text->segname    = "__TEXT";
    sg_text->maxprot    = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
    sg_text->initprot   = VM_PROT_READ | VM_PROT_EXECUTE;

    sc_text->sectname   = "__text";
    sc_text->flags      = S_REGULAR;

    sg_rodata->segname  = "__RODATA";
    sg_rodata->maxprot  = VM_PROT_READ | VM_PROT_EXECUTE;
    sg_rodata->initprot = VM_PROT_READ;

    sc_rodata->sectname = "__rodata";

    sg_data->segname    = "__DATA";
    sg_data->maxprot    = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
    sg_data->initprot   = VM_PROT_READ | VM_PROT_WRITE;

    sc_data->sectname   = "__data";

    sc_pinit->sectname  = "__preinit_arr";
    sc_pinit->flags     = S_MOD_INIT_FUNC_POINTERS;

    sc_init->sectname   = "__init_arr";
    sc_init->flags      = S_MOD_INIT_FUNC_POINTERS;

    sc_fini->sectname   = "__fini_arr";
    sc_fini->flags      = S_MOD_TERM_FUNC_POINTERS;

    sg_bss->segname     = "__BSS";
    sg_bss->maxprot     = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
    sg_bss->initprot    = VM_PROT_READ | VM_PROT_WRITE;

    sc_bss->sectname    = "__bss";
    sc_bss->flags       = S_ZEROFILL;

    switch (m->header.cpu.type)
    {
    case CPU_TYPE_X86:
        {
            auto state = th_unix->add_state<thread_state_x86>();
            state->eip = eh.e_entry;
        }
        break;
    case CPU_TYPE_X86_64:
        {
            auto state = th_unix->add_state<thread_state_x86_64>();
            state->rip = eh.e_entry;
        }
        break;
    case CPU_TYPE_POWERPC:
        {
            auto state = th_unix->add_state<thread_state_powerpc>();
            state->srr0 = eh.e_entry;
        }
        break;
    default:
        throw convert_exception("unimplemented CPU type support");
    }

    uint64_t origin;
    for (int i = 0; i < eh.e_shnum; ++i) {
        Elf_Scn *es;
        GElf_Shdr esh;

        if ((es = elf_getscn(elf, i)) == NULL)
            throw new convert_exception("unable to get ELF section");
        if (gelf_getshdr(es, &esh) == NULL)
            throw new convert_exception("unable to get ELF section header");

        switch (i)
        {
        case 0: // NULL
            if (esh.sh_type != SHT_NULL)
                throw new convert_exception("ELF section 0 is not NULL");
            break;

        case 1: // .text
            if ((esh.sh_type != SHT_PROGBITS)
                    || (esh.sh_flags != (SHF_ALLOC|SHF_EXECINSTR))
                    || !CHECK_SEC_NAME(".text"))
                throw new convert_exception("ELF section 1 is not .text");
            origin = esh.sh_addr - esh.sh_offset;
            // we can set pagezero's size now that we know the origin
            sg_pgzero->vmsize = origin;
            // we can set the text segment
            sg_text->vmaddr = origin;
            sg_text->vmsize = ROUND_TO_2POW(MACH_ALIGN, esh.sh_size);
            sg_text->filesize = sg_text->vmsize;
            // we can set the text section
            sc_text->addr = esh.sh_addr;
            sc_text->size = esh.sh_size;
            sc_text->offset = esh.sh_offset;
            sc_text->align = (int)log2(esh.sh_addralign);
            break;

        case 2: // .rodata
            if ((esh.sh_type != SHT_PROGBITS)
                    || (esh.sh_flags != SHF_ALLOC)
                    || !CHECK_SEC_NAME(".rodata"))
                throw new convert_exception("ELF section 2 is not .rodata");
            // we can set the rodata segment
            sg_rodata->vmaddr = esh.sh_addr;
            sg_rodata->vmsize = ROUND_TO_2POW(MACH_ALIGN, esh.sh_size);
            sg_rodata->fileoff = esh.sh_offset;
            sg_rodata->filesize = sg_rodata->vmsize;
            // we can set the rodata section
            sc_rodata->addr = esh.sh_addr;
            sc_rodata->size = esh.sh_size;
            sc_rodata->offset = esh.sh_offset;
            sc_rodata->align = (int)log2(esh.sh_addralign);
            break;

        case 3: // .data
            if ((esh.sh_type != SHT_PROGBITS)
                    || (esh.sh_flags != (SHF_ALLOC|SHF_WRITE))
                    || !CHECK_SEC_NAME(".data"))
                throw new convert_exception("ELF section 3 is not .data");
            // we can set the data segment
            sg_data->vmaddr = esh.sh_addr;
            sg_data->vmsize = esh.sh_size;
            sg_data->fileoff = esh.sh_offset;
            // we can set the data section
            sc_data->addr = esh.sh_addr;
            sc_data->size = esh.sh_size;
            sc_data->offset = esh.sh_offset;
            sc_data->align = (int)log2(esh.sh_addralign);
            break;

        case 4: // .preinit_array
            if ((esh.sh_type != SHT_PREINIT_ARRAY)
                    || (esh.sh_flags != (SHF_ALLOC|SHF_WRITE))
                    || !CHECK_SEC_NAME(".preinit_array"))
                throw new convert_exception("ELF section 4 is not .preinit_array");
            // update the data segment
            sg_data->vmsize += esh.sh_size;
            // we can set the preinit section
            sc_pinit->addr = esh.sh_addr;
            sc_pinit->size = esh.sh_size;
            sc_pinit->offset = esh.sh_offset;
            sc_pinit->align = (int)log2(esh.sh_addralign);
            break;

        case 5: // .init_array
            if ((esh.sh_type != SHT_INIT_ARRAY)
                    || (esh.sh_flags != (SHF_ALLOC|SHF_WRITE))
                    || !CHECK_SEC_NAME(".init_array"))
                throw new convert_exception("ELF section 5 is not .init_array");
            // update the data segment
            sg_data->vmsize += esh.sh_size;
            // we can set the init section
            sc_init->addr = esh.sh_addr;
            sc_init->size = esh.sh_size;
            sc_init->offset = esh.sh_offset;
            sc_init->align = (int)log2(esh.sh_addralign);
            break;

        case 6: // .fini_array
            if ((esh.sh_type != SHT_FINI_ARRAY)
                    || (esh.sh_flags != (SHF_ALLOC|SHF_WRITE))
                    || !CHECK_SEC_NAME(".fini_array"))
                throw new convert_exception("ELF section 6 is not .fini_array");
            // update the data segment; we can finalize its size now
            sg_data->vmsize += esh.sh_size;
            sg_data->vmsize = ROUND_TO_2POW(MACH_ALIGN, sg_data->vmsize);
            sg_data->filesize = sg_data->vmsize;
            // we can set the fini section
            sc_fini->addr = esh.sh_addr;
            sc_fini->size = esh.sh_size;
            sc_fini->offset = esh.sh_offset;
            sc_fini->align = (int)log2(esh.sh_addralign);
            break;

        case 7: // .bss
            if ((esh.sh_type != SHT_NOBITS)
                    || (esh.sh_flags != (SHF_ALLOC|SHF_WRITE))
                    || !CHECK_SEC_NAME(".bss"))
                throw new convert_exception("ELF section 7 is not .bss");
            // we can set the bss segment
            sg_bss->vmaddr = esh.sh_addr;
            sg_bss->vmsize = esh.sh_size;
            // we can set the bss section
            sc_bss->addr = esh.sh_addr;
            sc_bss->size = esh.sh_size;
            sc_bss->align = (int)log2(esh.sh_addralign);
            break;

        default:
            if (esh.sh_flags & SHF_ALLOC)
                throw new convert_exception("unhandled ELF sections marked as allocated");
            break;
        }
    }

    mach_file_segment fsg_data(sg_data->fileoff, sg_data->vmsize, NULL, 0);

    for (int i = 0; i < eh.e_shnum; ++i) {
        Elf_Scn *es;
        GElf_Shdr esh;
        Elf_Data *ed;

        if ((es = elf_getscn(elf, i)) == NULL)
            throw new convert_exception("unable to get ELF section");
        if (gelf_getshdr(es, &esh) == NULL)
            throw new convert_exception("unable to get ELF section header");

        switch (i)
        {
        case 1:
            {
                // actual size is minus the headers, since they are in this
                // section too, because... reasons
                size_t real_size = sg_text->vmsize - m->size_of_headers();

                // get the data from our elf section
                if ((ed = elf_rawdata(es, NULL)) == NULL)
                    throw new convert_exception("unable to get ELF section data");
                if (ed->d_size > real_size)
                    throw new convert_exception("ELF .text section too big");

                // add file segment to mach
                m->add_file_segment({
                    sc_text->offset, real_size,
                    (char*)ed->d_buf, ed->d_size
                });

                // make sure the ELF section isn't split...
                if ((ed = elf_rawdata(es, ed)) != NULL)
                    throw new convert_exception("ELF .text section split");
            }
            break;

        case 2:
            {
                // get the data from our elf section
                if ((ed = elf_rawdata(es, NULL)) == NULL)
                    throw new convert_exception("unable to get ELF section data");
                if (ed->d_size > sg_rodata->vmsize)
                    throw new convert_exception("ELF .rodata section too big");

                // add file segment to mach
                m->add_file_segment({
                    sg_rodata->fileoff, sg_rodata->vmsize,
                    (char*)ed->d_buf, ed->d_size
                });

                // make sure the ELF section isn't split...
                if ((ed = elf_rawdata(es, ed)) != NULL)
                    throw new convert_exception("ELF .rodata section split");
            }
            break;

        case 3:
            {
                // get the data from our elf section
                if ((ed = elf_rawdata(es, NULL)) == NULL)
                    throw new convert_exception("unable to get ELF section data");
                if (ed->d_size > (sg_data->vmsize - sc_data->offset))
                    throw new convert_exception("ELF .data section too big");

                // copy data to segment
                memcpy(fsg_data.content() + (sc_data->offset - sg_data->fileoff),
                       ed->d_buf, ed->d_size);

                // make sure the ELF section isn't split...
                if ((ed = elf_rawdata(es, ed)) != NULL)
                    throw new convert_exception("ELF .data section split");
            }
            break;

        case 4:
            {
                // get the data from our elf section
                if ((ed = elf_rawdata(es, NULL)) == NULL)
                    throw new convert_exception("unable to get ELF section data");
                if (ed->d_size > (sg_data->vmsize - sc_pinit->offset))
                    throw new convert_exception("ELF .preinit_array section too big");

                // copy data to segment
                memcpy(fsg_data.content() + (sc_pinit->offset - sg_data->fileoff),
                       ed->d_buf, ed->d_size);

                // make sure the ELF section isn't split...
                if ((ed = elf_rawdata(es, ed)) != NULL)
                    throw new convert_exception("ELF .preinit_array section split");
            }
            break;

        case 5:
            {
                // get the data from our elf section
                if ((ed = elf_rawdata(es, NULL)) == NULL)
                    throw new convert_exception("unable to get ELF section data");
                if (ed->d_size > (sg_data->vmsize - sc_init->offset))
                    throw new convert_exception("ELF .init_array section too big");

                // copy data to segment
                memcpy(fsg_data.content() + (sc_init->offset - sg_data->fileoff),
                       ed->d_buf, ed->d_size);

                // make sure the ELF section isn't split...
                if ((ed = elf_rawdata(es, ed)) != NULL)
                    throw new convert_exception("ELF .init_array section split");
            }
            break;

        case 6:
            {
                // get the data from our elf section
                if ((ed = elf_rawdata(es, NULL)) == NULL)
                    throw new convert_exception("unable to get ELF section data");
                if (ed->d_size > (sg_data->vmsize - sc_fini->offset))
                    throw new convert_exception("ELF .fini_array section too big");

                // copy data to segment
                memcpy(fsg_data.content() + (sc_fini->offset - sg_data->fileoff),
                       ed->d_buf, ed->d_size);
                m->add_file_segment(fsg_data);

                // make sure the ELF section isn't split...
                if ((ed = elf_rawdata(es, ed)) != NULL)
                    throw new convert_exception("ELF .fini_array section split");
            }
            break;
        }
    }

    if ((sc_text->addr - origin) != m->size_of_headers()) {
        cerr << "!! offset of .text (" << hex << (sc_text->addr - origin)
             << ") != header size (" << hex << m->size_of_headers() << ")\n",
        throw new convert_exception("Mach header misalignment");
    }

    return m;
}


#define FAT_MAGIC 0xcafebabe
#define FAT_HEADER_POS(i) \
    ((2*4) + ((5*4) * (i)))

void make_fat(vector<shared_ptr<mach>>& arches, const char *outfile)
{
    // if we weren't given any binaries, die?
    if (arches.size() == 0)
        throw new convert_exception("no binares to merge");

    // open our output file
    ofstream os(outfile);

    // if we were only given one binary, write it non-fat
    if (arches.size() == 1) {
        arches[0]->write(os);

    } else {
        // write the initial fat binary header
        _write(os, MACH_MSB, (uint32_t)FAT_MAGIC);
        _write(os, MACH_MSB, (uint32_t)arches.size());

        // iterate through the executables we were told to zip up and write them
        size_t last = MACH_ALIGN, size;
        for (int i = 0; i < arches.size(); ++i) {
            // write the mach-o at the next place available in the file
            os.seekp(last, os.beg);
            arches[i]->write(os);
            size = (size_t)os.tellp() - last;
            // now that we know how large the executable is, write its header
            os.seekp(FAT_HEADER_POS(i), os.beg);
            _write(os, MACH_MSB, arches[i]->header.cpu.type);
            _write(os, MACH_MSB, arches[i]->header.cpu.subtype);
            _write(os, MACH_MSB, (uint32_t)last);
            _write(os, MACH_MSB, (uint32_t)size);
            _write(os, MACH_MSB, (uint32_t)0);
            // increment last by size, but round to page size
            last = ROUND_TO_2POW(MACH_ALIGN, (last + size));
        }
    }

    // close the file stream and make our output file executable
    os.close();
    chmod(outfile, MODE_X);
}


Elf *init(const char *filename, int mode)
{
    int fd;
    Elf_Cmd elfmode;
    Elf *elf;

    if (elf_version(EV_CURRENT) == EV_NONE) {
        progerr << "ELF library version mismatch" << endl;
        return NULL;
    }

    switch (mode)
    {
    case O_RDONLY:
        elfmode = ELF_C_READ;
        break;
    case O_WRONLY:
        elfmode = ELF_C_WRITE;
        break;
    case O_RDWR:
        elfmode = ELF_C_RDWR;
        break;
    default:
        progerr << "unknown file mode" << endl;
        return NULL;
    }

    fd = open(filename, mode);
    if (fd == -1) {
        progerr << "unable to open '" << filename << "'" << endl;
        return NULL;
    }
    elf = elf_begin(fd, elfmode, (Elf *)NULL);

    if (elf_kind(elf) != ELF_K_ELF) {
        progerr << "'" << filename << "' is not an ELF file" << endl;
        elf_end(elf);
        return NULL;
    }

    return elf;
}


void usage()
{
    cout << "Usage: " << progname << " [-h] [-o macho-file] elf-file" << endl;
}


int main(int argc, char **argv)
{
    int c, r = 0;
    const char* outfile = "macho";

    progname = argv[0];
    while ((c = getopt(argc, argv, "ho:")) != -1) {
        switch (c)
        {
        case 'h':
            usage();
            return 0;
        case 'o':
            outfile = optarg;
            break;
        case '?':
            switch (optopt)
            {
            case 'o':
                break;
            default:
                progerr << "unknown option '-" << optopt << "'" << endl;
            }
            return 1;
        default:
            abort();
        }
    }

    // make sure we have some inputs...
    if (optind >= argc) {
        progerr << "input file(s) must be specified" << endl;
        return 1;
    }

    // generate converted mach-o binaries for each elf input
    vector<shared_ptr<mach>> arches;
    for (int i = optind; i < argc; ++i) {
        Elf *elf;
        try {
            elf = init(argv[i], O_RDONLY);
            arches.push_back(convert(elf));
        } catch (const convert_exception *exc) {
            progerr << "failed to convert '" << argv[i] << "': " << exc->what() << endl;
            r = 1;
        }
        if (elf != NULL)
            elf_end(elf);
    }

    // now that we have all our binaries ready, output them
    try {
        make_fat(arches, outfile);
    } catch (const convert_exception *exc) {
        progerr << "failed to output binary: " << exc->what() << endl;
        r = 1;
    }

    return r;
}


