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

#ifndef _MACHO_H
#define _MACHO_H 1

#include <cstdint>

typedef int32_t     cpu_type_t;
typedef int32_t     cpu_subtype_t;
typedef int32_t     cpu_threadtype_t;

#define CPU_ARCH_MASK       0xff000000
#define CPU_ARCH_ABI64      0x01000000  /* 64-bit */
#define CPU_ARCH_ABI64_32   0x02000000  /* 32-on-64; LP32 */

#define CPU_TYPE_ANY        ((cpu_type_t) -1)
#define CPU_TYPE_VAX        ((cpu_type_t)  1)
#define CPU_TYPE_MC680x0	((cpu_type_t)  6)
#define CPU_TYPE_X86        ((cpu_type_t)  7)
#define CPU_TYPE_X86_64     (CPU_TYPE_X86 | CPU_ARCH_ABI64)
#define CPU_TYPE_MIPS       ((cpu_type_t)  8)
#define CPU_TYPE_MC98000    ((cpu_type_t) 10)
#define CPU_TYPE_HPPA       ((cpu_type_t) 11)
#define CPU_TYPE_ARM        ((cpu_type_t) 12)
#define CPU_TYPE_ARM64      (CPU_TYPE_ARM | CPU_ARCH_ABI64)
#define CPU_TYPE_MC88000    ((cpu_type_t) 13)
#define CPU_TYPE_SPARC      ((cpu_type_t) 14)
#define CPU_TYPE_I860       ((cpu_type_t) 15)
#define CPU_TYPE_ALPHA      ((cpu_type_t) 16)
#define CPU_TYPE_POWERPC    ((cpu_type_t) 18)
#define CPU_TYPE_POWERPC64  (CPU_TYPE_POWERPC | CPU_ARCH_ABI64)

#define CPU_SUBTYPE_MASK    0xff000000
#define CPU_SUBTYPE_LIB64   0x80000000  /* 64 bit libraries */

#define CPU_SUBTYPE_MULTIPLE        ((cpu_subtype_t) -1) /* either endian */
#define CPU_SUBTYPE_LITTLE_ENDIAN   ((cpu_subtype_t) 0)
#define CPU_SUBTYPE_BIG_ENDIAN      ((cpu_subtype_t) 1)

#define CPU_SUBTYPE_VAX_ALL         ((cpu_subtype_t) 0) 
#define CPU_SUBTYPE_VAX780          ((cpu_subtype_t) 1)
#define CPU_SUBTYPE_VAX785          ((cpu_subtype_t) 2)
#define CPU_SUBTYPE_VAX750          ((cpu_subtype_t) 3)
#define CPU_SUBTYPE_VAX730          ((cpu_subtype_t) 4)
#define CPU_SUBTYPE_UVAXI           ((cpu_subtype_t) 5)
#define CPU_SUBTYPE_UVAXII          ((cpu_subtype_t) 6)
#define CPU_SUBTYPE_VAX8200         ((cpu_subtype_t) 7)
#define CPU_SUBTYPE_VAX8500         ((cpu_subtype_t) 8)
#define CPU_SUBTYPE_VAX8600         ((cpu_subtype_t) 9)
#define CPU_SUBTYPE_VAX8650         ((cpu_subtype_t) 10)
#define CPU_SUBTYPE_VAX8800         ((cpu_subtype_t) 11)
#define CPU_SUBTYPE_UVAXIII         ((cpu_subtype_t) 12)

#define CPU_SUBTYPE_MC680x0_ALL     ((cpu_subtype_t) 1)
#define CPU_SUBTYPE_MC68040         ((cpu_subtype_t) 2) 
#define CPU_SUBTYPE_MC68030_ONLY    ((cpu_subtype_t) 3)

#define CPU_SUBTYPE_INTEL(f, m)     ((cpu_subtype_t)(f) + ((m) << 4))

#define CPU_SUBTYPE_X86_ALL         CPU_SUBTYPE_INTEL( 3, 0)
#define CPU_SUBTYPE_386             CPU_SUBTYPE_INTEL( 3, 0)
#define CPU_SUBTYPE_486             CPU_SUBTYPE_INTEL( 4, 0)
#define CPU_SUBTYPE_486SX           CPU_SUBTYPE_INTEL( 4, 8)
#define CPU_SUBTYPE_586             CPU_SUBTYPE_INTEL( 5, 0)
#define CPU_SUBTYPE_PENT            CPU_SUBTYPE_INTEL( 5, 0)
#define CPU_SUBTYPE_PENTPRO         CPU_SUBTYPE_INTEL( 6, 1)
#define CPU_SUBTYPE_PENTII_M3       CPU_SUBTYPE_INTEL( 6, 3)
#define CPU_SUBTYPE_PENTII_M5       CPU_SUBTYPE_INTEL( 6, 5)
#define CPU_SUBTYPE_CELERON         CPU_SUBTYPE_INTEL( 7, 6)
#define CPU_SUBTYPE_CELERON_MOBILE  CPU_SUBTYPE_INTEL( 7, 7)
#define CPU_SUBTYPE_PENTIUM_3       CPU_SUBTYPE_INTEL( 8, 0)
#define CPU_SUBTYPE_PENTIUM_3_M     CPU_SUBTYPE_INTEL( 8, 1)
#define CPU_SUBTYPE_PENTIUM_3_XEON  CPU_SUBTYPE_INTEL( 8, 2)
#define CPU_SUBTYPE_PENTIUM_M       CPU_SUBTYPE_INTEL( 9, 0)
#define CPU_SUBTYPE_PENTIUM_4       CPU_SUBTYPE_INTEL(10, 0)
#define CPU_SUBTYPE_PENTIUM_4_M     CPU_SUBTYPE_INTEL(10, 1)
#define CPU_SUBTYPE_ITANIUM         CPU_SUBTYPE_INTEL(11, 0)
#define CPU_SUBTYPE_ITANIUM_2       CPU_SUBTYPE_INTEL(11, 1)
#define CPU_SUBTYPE_XEON            CPU_SUBTYPE_INTEL(12, 0)
#define CPU_SUBTYPE_XEON_MP         CPU_SUBTYPE_INTEL(12, 1)

#define CPU_SUBTYPE_INTEL_FAMILY(x)     ((x) & 15)
#define CPU_SUBTYPE_INTEL_FAMILY_MAX    15

#define CPU_SUBTYPE_INTEL_MODEL(x)      ((x) >> 4)
#define CPU_SUBTYPE_INTEL_MODEL_ALL     0

#define CPU_SUBTYPE_X86_64_ALL      CPU_SUBTYPE_INTEL( 3, 0)

#define	CPU_SUBTYPE_MIPS_ALL        ((cpu_subtype_t) 0)
#define CPU_SUBTYPE_MIPS_R2300      ((cpu_subtype_t) 1)
#define CPU_SUBTYPE_MIPS_R2600      ((cpu_subtype_t) 2)
#define CPU_SUBTYPE_MIPS_R2800      ((cpu_subtype_t) 3)
#define CPU_SUBTYPE_MIPS_R2000a     ((cpu_subtype_t) 4)
#define CPU_SUBTYPE_MIPS_R2000      ((cpu_subtype_t) 5)
#define CPU_SUBTYPE_MIPS_R3000a     ((cpu_subtype_t) 6)
#define CPU_SUBTYPE_MIPS_R3000      ((cpu_subtype_t) 7)

#define CPU_SUBTYPE_MC98000_ALL     ((cpu_subtype_t) 0)
#define CPU_SUBTYPE_MC98601         ((cpu_subtype_t) 1)

#define CPU_SUBTYPE_HPPA_ALL        ((cpu_subtype_t) 0)
#define CPU_SUBTYPE_HPPA_7100LC     ((cpu_subtype_t) 1)

#define CPU_SUBTYPE_MC88000_ALL     ((cpu_subtype_t) 0)
#define CPU_SUBTYPE_MC88100         ((cpu_subtype_t) 1)
#define CPU_SUBTYPE_MC88110         ((cpu_subtype_t) 2)

#define CPU_SUBTYPE_SPARC_ALL       ((cpu_subtype_t) 0)

#define CPU_SUBTYPE_I860_ALL        ((cpu_subtype_t) 0)
#define CPU_SUBTYPE_I860_860        ((cpu_subtype_t) 1)

#define CPU_SUBTYPE_ALPHA_ALL       ((cpu_subtype_t) 0)

#define CPU_SUBTYPE_POWERPC_ALL     ((cpu_subtype_t) 0)
#define CPU_SUBTYPE_POWERPC_601     ((cpu_subtype_t) 1)
#define CPU_SUBTYPE_POWERPC_602     ((cpu_subtype_t) 2)
#define CPU_SUBTYPE_POWERPC_603     ((cpu_subtype_t) 3)
#define CPU_SUBTYPE_POWERPC_603e    ((cpu_subtype_t) 4)
#define CPU_SUBTYPE_POWERPC_603ev   ((cpu_subtype_t) 5)
#define CPU_SUBTYPE_POWERPC_604     ((cpu_subtype_t) 6)
#define CPU_SUBTYPE_POWERPC_604e    ((cpu_subtype_t) 7)
#define CPU_SUBTYPE_POWERPC_620     ((cpu_subtype_t) 8)
#define CPU_SUBTYPE_POWERPC_750     ((cpu_subtype_t) 9)
#define CPU_SUBTYPE_POWERPC_7400    ((cpu_subtype_t) 10)
#define CPU_SUBTYPE_POWERPC_7450    ((cpu_subtype_t) 11)
#define CPU_SUBTYPE_POWERPC_970     ((cpu_subtype_t) 100)

#define CPU_SUBTYPE_POWERPC64_ALL   ((cpu_subtype_t) 0)

#define CPU_SUBTYPE_ARM_ALL         ((cpu_subtype_t) 0)
#define CPU_SUBTYPE_ARM_V4T         ((cpu_subtype_t) 5)
#define CPU_SUBTYPE_ARM_V6          ((cpu_subtype_t) 6)
#define CPU_SUBTYPE_ARM_V5TEJ       ((cpu_subtype_t) 7)
#define CPU_SUBTYPE_ARM_XSCALE      ((cpu_subtype_t) 8)
#define CPU_SUBTYPE_ARM_V7          ((cpu_subtype_t) 9)

#define CPU_SUBTYPE_ARM64_ALL       ((cpu_subtype_t) 0)

#define MH_MAGIC        0xfeedface  /* the mach magic number */
#define MH_MAGIC_64     0xfeedfacf  /* the 64-bit mach magic number */

#define MH_OBJECT       0x1
#define MH_EXECUTE      0x2
#define MH_FVMLIB       0x3
#define MH_CORE         0x4
#define MH_PRELOAD      0x5
#define MH_DYLIB        0x6
#define MH_DYLINKER     0x7
#define MH_BUNDLE       0x8
#define MH_DYLIB_STUB   0x9
#define MH_DSYM         0xa
#define MH_KEXT_BUNDLE  0xb
#define MH_FILESET      0xc

#define MH_NOUNDEFS                         0x00000001
#define MH_INCRLINK                         0x00000002
#define MH_DYLDLINK                         0x00000004
#define MH_BINDATLOAD                       0x00000008
#define MH_PREBOUND                         0x00000010
#define MH_SPLIT_SEGS                       0x00000020
#define MH_LAZY_INIT                        0x00000040
#define MH_TWOLEVEL                         0x00000080
#define MH_FORCE_FLAT                       0x00000100
#define MH_NOMULTIDEFS                      0x00000200
#define MH_NOFIXPREBINDING                  0x00000400
#define MH_PREBINDABLE                      0x00000800
#define MH_ALLMODSBOUND                     0x00001000
#define MH_SUBSECTIONS_VIA_SYMBOLS          0x00002000
#define MH_CANONICAL                        0x00004000
#define MH_WEAK_DEFINES                     0x00008000
#define MH_BINDS_TO_WEAK                    0x00010000
#define MH_ALLOW_STACK_EXECUTION            0x00020000
#define MH_ROOT_SAFE                        0x00040000
#define MH_SETUID_SAFE                      0x00080000
#define MH_NO_REEXPORTED_DYLIBS             0x00100000
#define MH_PIE                              0x00200000
#define MH_DEAD_STRIPPABLE_DYLIB            0x00400000
#define MH_HAS_TLV_DESCRIPTORS              0x00800000
#define MH_NO_HEAP_EXECUTION                0x01000000
#define MH_APP_EXTENSION_SAFE               0x02000000
#define MH_NLIST_OUTOFSYNC_WITH_DYLDINFO    0x04000000
#define MH_SIM_SUPPORT                      0x08000000
#define MH_DYLIB_IN_CACHE                   0x80000000

#define LC_REQ_DYLD                 0x80000000

#define LC_SEGMENT                  0x01
#define LC_SYMTAB                   0x02
#define LC_SYMSEG                   0x03
#define LC_THREAD                   0x04
#define LC_UNIXTHREAD               0x05
#define LC_LOADFVMLIB               0x06
#define LC_IDFVMLIB                 0x07
#define LC_IDENT                    0x08
#define LC_FVMFILE                  0x09
#define LC_PREPAGE                  0x0a
#define LC_DYSYMTAB                 0x0b
#define LC_LOAD_DYLIB               0x0c
#define LC_ID_DYLIB                 0x0d
#define LC_LOAD_DYLINKER            0x0e
#define LC_ID_DYLINKER              0x0f
#define LC_PREBOUND_DYLIB           0x10
#define LC_ROUTINES                 0x11
#define LC_SUB_FRAMEWORK            0x12
#define LC_SUB_UMBRELLA             0x13
#define LC_SUB_CLIENT               0x14
#define LC_SUB_LIBRARY              0x15
#define LC_TWOLEVEL_HINTS           0x16
#define LC_PREBIND_CKSUM            0x17
#define LC_LOAD_WEAK_DYLIB         (0x18 | LC_REQ_DYLD)
#define LC_SEGMENT_64               0x19
#define LC_ROUTINES_64              0x1a
#define LC_UUID                     0x1b
#define LC_RPATH                   (0x1c | LC_REQ_DYLD)
#define LC_CODE_SIGNATURE           0x1d
#define LC_SEGMENT_SPLIT_INFO       0x1e
#define LC_REEXPORT_DYLIB          (0x1f | LC_REQ_DYLD)
#define LC_LAZY_LOAD_DYLIB          0x20
#define LC_ENCRYPTION_INFO          0x21
#define LC_DYLD_INFO                0x22
#define LC_DYLD_INFO_ONLY          (0x22 | LC_REQ_DYLD)
#define LC_LOAD_UPWARD_DYLIB       (0x23 | LC_REQ_DYLD)
#define LC_VERSION_MIN_MACOSX       0x24
#define LC_VERSION_MIN_IPHONEOS     0x25
#define LC_FUNCTION_STARTS          0x26
#define LC_DYLD_ENVIRONMENT         0x27
#define LC_MAIN                    (0x28 | LC_REQ_DYLD)
#define LC_DATA_IN_CODE             0x29
#define LC_SOURCE_VERSION           0x2A
#define LC_DYLIB_CODE_SIGN_DRS      0x2B
#define LC_ENCRYPTION_INFO_64       0x2C
#define LC_LINKER_OPTION            0x2D
#define LC_LINKER_OPTIMIZATION_HINT 0x2E
#define LC_VERSION_MIN_TVOS         0x2F
#define LC_VERSION_MIN_WATCHOS      0x30
#define LC_NOTE                     0x31
#define LC_BUILD_VERSION            0x32
#define LC_DYLD_EXPORTS_TRIE       (0x33 | LC_REQ_DYLD)
#define LC_DYLD_CHAINED_FIXUPS     (0x34 | LC_REQ_DYLD)
#define LC_FILESET_ENTRY           (0x35 | LC_REQ_DYLD)

typedef uint32_t lc_str; /* offset to an embedded string string */

typedef uint32_t vm_prot_t;

#define VM_PROT_NONE        ((vm_prot_t) 0x00)
#define VM_PROT_READ        ((vm_prot_t) 0x01)
#define VM_PROT_WRITE       ((vm_prot_t) 0x02)
#define VM_PROT_EXECUTE     ((vm_prot_t) 0x04)

#define SG_HIGHVM               0x01
#define SG_FVMLIB               0x02
#define SG_NORELOC              0x04
#define SG_PROTECTED_VERSION_1  0x08
#define SG_READ_ONLY            0x10

#define SECTION_TYPE_MASK                       0x000000ff
#define SECTION_ATTR_MASK                       0xffffff00
#define SECTION_ATTR_USR_MASK                   0xff000000
#define SECTION_ATTR_SYS_MASK                   0x00ffff00

#define S_REGULAR                               0x00
#define S_ZEROFILL                              0x01
#define S_CSTRING_LITERALS                      0x02
#define S_4BYTE_LITERALS                        0x03
#define S_8BYTE_LITERALS                        0x04
#define S_LITERAL_POINTERS                      0x05
#define S_NON_LAZY_SYMBOL_POINTERS              0x06
#define S_LAZY_SYMBOL_POINTERS                  0x07
#define S_SYMBOL_STUBS                          0x08
#define S_MOD_INIT_FUNC_POINTERS                0x09
#define S_MOD_TERM_FUNC_POINTERS                0x0a
#define S_COALESCED                             0x0b
#define S_GB_ZEROFILL                           0x0c
#define S_INTERPOSING                           0x0d
#define S_16BYTE_LITERALS                       0x0e
#define S_DTRACE_DOF                            0x0f
#define S_LAZY_DYLIB_SYMBOL_POINTERS            0x10
#define S_THREAD_LOCAL_REGULAR                  0x11
#define S_THREAD_LOCAL_ZEROFILL                 0x12
#define S_THREAD_LOCAL_VARIABLES                0x13
#define S_THREAD_LOCAL_VARIABLE_POINTERS        0x14
#define S_THREAD_LOCAL_INIT_FUNCTION_POINTERS   0x15
#define S_INIT_FUNC_OFFSETS                     0x16

#define S_ATTR_PURE_INSTRUCTIONS                0x80000000
#define S_ATTR_NO_TOC                           0x40000000
#define S_ATTR_STRIP_STATIC_SYMS                0x20000000
#define S_ATTR_NO_DEAD_STRIP                    0x10000000
#define S_ATTR_LIVE_SUPPORT                     0x08000000
#define S_ATTR_SELF_MODIFYING_CODE              0x04000000
#define S_ATTR_DEBUG                            0x02000000
#define S_ATTR_SOME_INSTRUCTIONS                0x00000400
#define S_ATTR_EXT_RELOC                        0x00000200
#define S_ATTR_LOC_RELOC                        0x00000100

#define PPC_THREAD_STATE        1
#define PPC_FLOAT_STATE         2
#define PPC_EXCEPTION_STATE     3
#define PPC_VECTOR_STATE        4
#define PPC_THREAD_STATE64      5
#define PPC_EXCEPTION_STATE64   6
#define PPC_THREAD_STATE_NONE   7

#define x86_THREAD_STATE32      1
#define x86_FLOAT_STATE32       2
#define x86_EXCEPTION_STATE32   3
#define x86_THREAD_STATE64      4
#define x86_FLOAT_STATE64       5
#define x86_EXCEPTION_STATE64   6
#define x86_THREAD_STATE        7
#define x86_FLOAT_STATE         8
#define x86_EXCEPTION_STATE     9
#define x86_DEBUG_STATE32       10
#define x86_DEBUG_STATE64       11
#define x86_DEBUG_STATE         12
#define x86_THREAD_STATE_NONE   13
#define x86_AVX_STATE32         16
#define x86_AVX_STATE64         17


//#define USER_CODE_SELECTOR      0x0017
//#define USER_DATA_SELECTOR      0x001f
//#define KERN_CODE_SELECTOR      0x0008
//#define KERN_DATA_SELECTOR      0x0010
//
//#define _THREAD_STATE_COUNT(s) \
//    ((uint32_t)(sizeof(s) / sizeof(uint32_t)))
//
///* x86 state structures */
//
//typedef struct _X86_FP_CONTROL {
//    unsigned short \
//        invalid :1,
//        denorm  :1,
//        zdiv    :1,
//        ovrfl   :1,
//        undfl   :1,
//        precis  :1,
//                :2,
//        pc      :2,
//        rc      :2,
//        /*inf*/ :1,
//                :3;
//} x86_fp_control_t;
//
//#define FP_PREC_24B 0
//#define FP_PREC_53B 2
//#define FP_PREC_64B 3
//#define FP_RND_NEAR 0
//#define FP_RND_DOWN 1
//#define FP_RND_UP   2
//#define FP_CHOP     3
//
//typedef struct _X86_FP_STATUS {
//    unsigned short \
//        invalid :1,
//        denorm  :1,
//        zdiv    :1,
//        ovrfl   :1,
//        undfl   :1,
//        precis  :1,
//        stkflt  :1,
//        errsumm :1,
//        c0      :1,
//        c1      :1,
//        c2      :1,
//        tos     :3,
//        c3      :1,
//        busy    :1;
//} x86_fp_status_t;
//
//typedef struct _X86_MMST_REG {
//    char mmst_reg[10];
//    char mmst_rsrv[6];
//} x86_mmst_reg_t;
//
//typedef struct _X86_XMM_REG {
//	char xmm_reg[16];
//} x86_xmm_reg_t;
//
//typedef struct _X86_THREAD_STATE32 {
//    uint32_t eax;
//    uint32_t ebx;
//    uint32_t ecx;
//    uint32_t edx;
//    uint32_t edi;
//    uint32_t esi;
//    uint32_t ebp;
//    uint32_t esp;
//    uint32_t ss;
//    uint32_t eflags;
//    uint32_t eip;
//    uint32_t cs;
//    uint32_t ds;
//    uint32_t es;
//    uint32_t fs;
//    uint32_t gs;
//} x86_thread_state32_t;
//
//typedef struct _X86_FLOAT_STATE {
//    int                 fpu_reserved[2];
//    x86_fp_control_t    fpu_fcw;            /* x87 FPU control word */
//    x86_fp_status_t     fpu_fsw;            /* x87 FPU status word */
//    uint8_t             fpu_ftw;            /* x87 FPU tag word */
//    uint8_t             fpu_rsrv1;          /* reserved */ 
//    uint16_t            fpu_fop;            /* x87 FPU Opcode */
//    uint32_t            fpu_ip;             /* x87 FPU Instruction Pointer offset */
//    uint16_t            fpu_cs;             /* x87 FPU Instruction Pointer Selector */
//    uint16_t            fpu_rsrv2;          /* reserved */
//    uint32_t            fpu_dp;             /* x87 FPU Instruction Operand(Data) Pointer offset */
//    uint16_t            fpu_ds;             /* x87 FPU Instruction Operand(Data) Pointer Selector */
//    uint16_t            fpu_rsrv3;          /* reserved */
//    uint32_t            fpu_mxcsr;          /* MXCSR Register state */
//    uint32_t            fpu_mxcsrmask;      /* MXCSR mask */
//    x86_mmst_reg_t      fpu_stmm0;          /* ST0/MM0   */
//    x86_mmst_reg_t      fpu_stmm1;          /* ST1/MM1  */
//    x86_mmst_reg_t      fpu_stmm2;          /* ST2/MM2  */
//    x86_mmst_reg_t      fpu_stmm3;          /* ST3/MM3  */
//    x86_mmst_reg_t      fpu_stmm4;          /* ST4/MM4  */
//    x86_mmst_reg_t      fpu_stmm5;          /* ST5/MM5  */
//    x86_mmst_reg_t      fpu_stmm6;          /* ST6/MM6  */
//    x86_mmst_reg_t      fpu_stmm7;          /* ST7/MM7  */
//    x86_xmm_reg_t       fpu_xmm0;           /* XMM 0  */
//    x86_xmm_reg_t       fpu_xmm1;           /* XMM 1  */
//    x86_xmm_reg_t       fpu_xmm2;           /* XMM 2  */
//    x86_xmm_reg_t       fpu_xmm3;           /* XMM 3  */
//    x86_xmm_reg_t       fpu_xmm4;           /* XMM 4  */
//    x86_xmm_reg_t       fpu_xmm5;           /* XMM 5  */
//    x86_xmm_reg_t       fpu_xmm6;           /* XMM 6  */
//    x86_xmm_reg_t       fpu_xmm7;           /* XMM 7  */
//    char                fpu_rsrv4[14*16];   /* reserved */
//    int                 fpu_reserved1;
//} x86_float_state32_t;
//
//typedef struct _X86_AVX_STATE32 {
//    int                 fpu_reserved[2];
//    x86_fp_control_t    fpu_fcw;            /* x87 FPU control word */
//    x86_fp_status_t     fpu_fsw;            /* x87 FPU status word */
//    uint8_t             fpu_ftw;            /* x87 FPU tag word */
//    uint8_t             fpu_rsrv1;          /* reserved */ 
//    uint16_t            fpu_fop;            /* x87 FPU Opcode */
//    uint32_t            fpu_ip;             /* x87 FPU Instruction Pointer offset */
//    uint16_t            fpu_cs;             /* x87 FPU Instruction Pointer Selector */
//    uint16_t            fpu_rsrv2;          /* reserved */
//    uint32_t            fpu_dp;             /* x87 FPU Instruction Operand(Data) Pointer offset */
//    uint16_t            fpu_ds;             /* x87 FPU Instruction Operand(Data) Pointer Selector */
//    uint16_t            fpu_rsrv3;          /* reserved */
//    uint32_t            fpu_mxcsr;          /* MXCSR Register state */
//    uint32_t            fpu_mxcsrmask;      /* MXCSR mask */
//    x86_mmst_reg_t      fpu_stmm0;          /* ST0/MM0   */
//    x86_mmst_reg_t      fpu_stmm1;          /* ST1/MM1  */
//    x86_mmst_reg_t      fpu_stmm2;          /* ST2/MM2  */
//    x86_mmst_reg_t      fpu_stmm3;          /* ST3/MM3  */
//    x86_mmst_reg_t      fpu_stmm4;          /* ST4/MM4  */
//    x86_mmst_reg_t      fpu_stmm5;          /* ST5/MM5  */
//    x86_mmst_reg_t      fpu_stmm6;          /* ST6/MM6  */
//    x86_mmst_reg_t      fpu_stmm7;          /* ST7/MM7  */
//    x86_xmm_reg_t       fpu_xmm0;           /* XMM 0  */
//    x86_xmm_reg_t       fpu_xmm1;           /* XMM 1  */
//    x86_xmm_reg_t       fpu_xmm2;           /* XMM 2  */
//    x86_xmm_reg_t       fpu_xmm3;           /* XMM 3  */
//    x86_xmm_reg_t       fpu_xmm4;           /* XMM 4  */
//    x86_xmm_reg_t       fpu_xmm5;           /* XMM 5  */
//    x86_xmm_reg_t       fpu_xmm6;           /* XMM 6  */
//    x86_xmm_reg_t       fpu_xmm7;           /* XMM 7  */
//    char                fpu_rsrv4[14*16];   /* reserved */
//    int                 fpu_reserved1;
//    char                __avx_reserved1[64];
//    x86_xmm_reg_t       __fpu_ymmh0;        /* YMMH 0  */
//    x86_xmm_reg_t       __fpu_ymmh1;        /* YMMH 1  */
//    x86_xmm_reg_t       __fpu_ymmh2;        /* YMMH 2  */
//    x86_xmm_reg_t       __fpu_ymmh3;        /* YMMH 3  */
//    x86_xmm_reg_t       __fpu_ymmh4;        /* YMMH 4  */
//    x86_xmm_reg_t       __fpu_ymmh5;        /* YMMH 5  */
//    x86_xmm_reg_t       __fpu_ymmh6;        /* YMMH 6  */
//    x86_xmm_reg_t       __fpu_ymmh7;        /* YMMH 7  */
//} x86_avx_state32_t;
//
//typedef struct _X86_EXCEPTION_STATE32 {
//    unsigned int trapno;
//    unsigned int err;
//    unsigned int faultvaddr;
//} x86_exception_state32_t;
//
//typedef struct _X86_DEBUG_STATE32 {
//    unsigned int  dr0;
//    unsigned int  dr1;
//    unsigned int  dr2;
//    unsigned int  dr3;
//    unsigned int  dr4;
//    unsigned int  dr5;
//    unsigned int  dr6;
//    unsigned int  dr7;
//} x86_debug_state32_t;
//
//
//typedef struct _X86_FLOAT_STATE64 {
//    int                 fpu_reserved[2];
//    x86_fp_control_t    fpu_fcw;            /* x87 control word */
//    x86_fp_status_t     fpu_fsw;            /* x87 status word */
//    uint8_t             fpu_ftw;            /* x87 tag word */
//    uint8_t             fpu_rsrv1;          /* reserved */ 
//    uint16_t            fpu_fop;            /* x87 opcode */
//    uint32_t            fpu_ip;             /* x87 inst. pointer offset */
//    uint16_t            fpu_cs;             /* x87 inst. pointer Selector */
//    uint16_t            fpu_rsrv2;          /* reserved */
//    uint32_t            fpu_dp;             /* x87 inst. op pointer offset */
//    uint16_t            fpu_ds;             /* x87 inst. op pointer selector */
//    uint16_t            fpu_rsrv3;          /* reserved */
//    uint32_t            fpu_mxcsr;          /* MXCSR Register state */
//    uint32_t            fpu_mxcsrmask;      /* MXCSR mask */
//    x86_mmst_reg_t      fpu_stmm0;          /* ST0/MM0   */
//    x86_mmst_reg_t      fpu_stmm1;          /* ST1/MM1  */
//    x86_mmst_reg_t      fpu_stmm2;          /* ST2/MM2  */
//    x86_mmst_reg_t      fpu_stmm3;          /* ST3/MM3  */
//    x86_mmst_reg_t      fpu_stmm4;          /* ST4/MM4  */
//    x86_mmst_reg_t      fpu_stmm5;          /* ST5/MM5  */
//    x86_mmst_reg_t      fpu_stmm6;          /* ST6/MM6  */
//    x86_mmst_reg_t      fpu_stmm7;          /* ST7/MM7  */
//    x86_xmm_reg_t       fpu_xmm0;           /* XMM 0  */
//    x86_xmm_reg_t       fpu_xmm1;           /* XMM 1  */
//    x86_xmm_reg_t       fpu_xmm2;           /* XMM 2  */
//    x86_xmm_reg_t       fpu_xmm3;           /* XMM 3  */
//    x86_xmm_reg_t       fpu_xmm4;           /* XMM 4  */
//    x86_xmm_reg_t       fpu_xmm5;           /* XMM 5  */
//    x86_xmm_reg_t       fpu_xmm6;           /* XMM 6  */
//    x86_xmm_reg_t       fpu_xmm7;           /* XMM 7  */
//    x86_xmm_reg_t       fpu_xmm8;           /* XMM 8  */
//    x86_xmm_reg_t       fpu_xmm9;           /* XMM 9  */
//    x86_xmm_reg_t       fpu_xmm10;          /* XMM 10  */
//    x86_xmm_reg_t       fpu_xmm11;          /* XMM 11 */
//    x86_xmm_reg_t       fpu_xmm12;          /* XMM 12  */
//    x86_xmm_reg_t       fpu_xmm13;          /* XMM 13  */
//    x86_xmm_reg_t       fpu_xmm14;          /* XMM 14  */
//    x86_xmm_reg_t       fpu_xmm15;          /* XMM 15  */
//    char                fpu_rsrv4[6*16];    /* reserved */
//    int                 fpu_reserved1;
//} x86_float_state64_t;
//
//typedef struct _X86_AVX_STATE64 {
//    int                 fpu_reserved[2];
//    x86_fp_control_t    fpu_fcw;            /* x87 FPU control word */
//    x86_fp_status_t     fpu_fsw;            /* x87 FPU status word */
//    uint8_t             fpu_ftw;            /* x87 FPU tag word */
//    uint8_t             fpu_rsrv1;          /* reserved */ 
//    uint16_t            fpu_fop;            /* x87 FPU Opcode */
//    uint32_t            fpu_ip;             /* x87 inst. pointer offset */
//    uint16_t            fpu_cs;             /* x87 inst. pointer Selector */
//    uint16_t            fpu_rsrv2;          /* reserved */
//    uint32_t            fpu_dp;             /* x86 inst. op pointer offset */
//    uint16_t            fpu_ds;             /* x86 inst. op pointer selector */
//    uint16_t            fpu_rsrv3;          /* reserved */
//    uint32_t            fpu_mxcsr;          /* MXCSR Register state */
//    uint32_t            fpu_mxcsrmask;      /* MXCSR mask */
//    x86_mmst_reg_t      fpu_stmm0;          /* ST0/MM0   */
//    x86_mmst_reg_t      fpu_stmm1;          /* ST1/MM1  */
//    x86_mmst_reg_t      fpu_stmm2;          /* ST2/MM2  */
//    x86_mmst_reg_t      fpu_stmm3;          /* ST3/MM3  */
//    x86_mmst_reg_t      fpu_stmm4;          /* ST4/MM4  */
//    x86_mmst_reg_t      fpu_stmm5;          /* ST5/MM5  */
//    x86_mmst_reg_t      fpu_stmm6;          /* ST6/MM6  */
//    x86_mmst_reg_t      fpu_stmm7;          /* ST7/MM7  */
//    x86_xmm_reg_t       fpu_xmm0;           /* XMM 0  */
//    x86_xmm_reg_t       fpu_xmm1;           /* XMM 1  */
//    x86_xmm_reg_t       fpu_xmm2;           /* XMM 2  */
//    x86_xmm_reg_t       fpu_xmm3;           /* XMM 3  */
//    x86_xmm_reg_t       fpu_xmm4;           /* XMM 4  */
//    x86_xmm_reg_t       fpu_xmm5;           /* XMM 5  */
//    x86_xmm_reg_t       fpu_xmm6;           /* XMM 6  */
//    x86_xmm_reg_t       fpu_xmm7;           /* XMM 7  */
//    x86_xmm_reg_t       fpu_xmm8;           /* XMM 8  */
//    x86_xmm_reg_t       fpu_xmm9;           /* XMM 9  */
//    x86_xmm_reg_t       fpu_xmm10;          /* XMM 10  */
//    x86_xmm_reg_t       fpu_xmm11;          /* XMM 11 */
//    x86_xmm_reg_t       fpu_xmm12;          /* XMM 12  */
//    x86_xmm_reg_t       fpu_xmm13;          /* XMM 13  */
//    x86_xmm_reg_t       fpu_xmm14;          /* XMM 14  */
//    x86_xmm_reg_t       fpu_xmm15;          /* XMM 15  */
//    char                fpu_rsrv4[6*16];    /* reserved */
//    int                 fpu_reserved1;
//    char                __avx_reserved1[64];
//    x86_xmm_reg_t       __fpu_ymmh0;        /* YMMH 0  */
//    x86_xmm_reg_t       __fpu_ymmh1;        /* YMMH 1  */
//    x86_xmm_reg_t       __fpu_ymmh2;        /* YMMH 2  */
//    x86_xmm_reg_t       __fpu_ymmh3;        /* YMMH 3  */
//    x86_xmm_reg_t       __fpu_ymmh4;        /* YMMH 4  */
//    x86_xmm_reg_t       __fpu_ymmh5;        /* YMMH 5  */
//    x86_xmm_reg_t       __fpu_ymmh6;        /* YMMH 6  */
//    x86_xmm_reg_t       __fpu_ymmh7;        /* YMMH 7  */
//    x86_xmm_reg_t       __fpu_ymmh8;        /* YMMH 8  */
//    x86_xmm_reg_t       __fpu_ymmh9;        /* YMMH 9  */
//    x86_xmm_reg_t       __fpu_ymmh10;       /* YMMH 10  */
//    x86_xmm_reg_t       __fpu_ymmh11;       /* YMMH 11  */
//    x86_xmm_reg_t       __fpu_ymmh12;       /* YMMH 12  */
//    x86_xmm_reg_t       __fpu_ymmh13;       /* YMMH 13  */
//    x86_xmm_reg_t       __fpu_ymmh14;       /* YMMH 14  */
//    x86_xmm_reg_t       __fpu_ymmh15;       /* YMMH 15  */
//} x86_avx_state64_t;
//
//typedef struct _X86_EXCEPTION_STATE64 {
//    unsigned int    trapno;
//    unsigned int    err;
//    uint64_t        faultvaddr;
//} x86_exception_state64_t;
//
//typedef struct _X86_DEBUG_STATE64 {
//    uint64_t dr0;
//    uint64_t dr1;
//    uint64_t dr2;
//    uint64_t dr3;
//    uint64_t dr4;
//    uint64_t dr5;
//    uint64_t dr6;
//    uint64_t dr7;
//} x86_debug_state64_t;
//
//#define PPC_THREAD_STATE        1
//#define PPC_FLOAT_STATE         2
//#define PPC_EXCEPTION_STATE     3
//#define PPC_VECTOR_STATE        4
//#define PPC_THREAD_STATE64      5
//#define PPC_EXCEPTION_STATE64   6
//#define PPC_THREAD_STATE_NONE   7
//
//typedef struct _STRUCT_PPC_THREAD_STATE {
//    unsigned int srr0;  /* Instruction address register (PC) */
//    unsigned int srr1;  /* Machine state register (supervisor) */
//    unsigned int r0;
//    unsigned int r1;
//    unsigned int r2;
//    unsigned int r3;
//    unsigned int r4;
//    unsigned int r5;
//    unsigned int r6;
//    unsigned int r7;
//    unsigned int r8;
//    unsigned int r9;
//    unsigned int r10;
//    unsigned int r11;
//    unsigned int r12;
//    unsigned int r13;
//    unsigned int r14;
//    unsigned int r15;
//    unsigned int r16;
//    unsigned int r17;
//    unsigned int r18;
//    unsigned int r19;
//    unsigned int r20;
//    unsigned int r21;
//    unsigned int r22;
//    unsigned int r23;
//    unsigned int r24;
//    unsigned int r25;
//    unsigned int r26;
//    unsigned int r27;
//    unsigned int r28;
//    unsigned int r29;
//    unsigned int r30;
//    unsigned int r31;
//
//    unsigned int cr;	/* Condition register */
//    unsigned int xer;	/* User's integer exception register */
//    unsigned int lr;	/* Link register */
//    unsigned int ctr;	/* Count register */
//    unsigned int mq;	/* MQ register (601 only) */
//
//    unsigned int vrsave;	/* Vector Save Register */
//} ppc_thread_state_t;
//
//struct symtab_command
//{
//    uint32_t cmd;       /* LC_SYMTAB */
//    uint32_t cmdsize;   /* sizeof(struct symtab_command) */
//    uint32_t symoff;    /* symbol table offset */
//    uint32_t nsyms;     /* number of symbol table entries */
//    uint32_t stroff;    /* string table offset */
//    uint32_t strsize;   /* string table size in bytes */
//};
//
//// symoff   d9e0
//// nsyms    ff
//// stroff   ead8
//// strsize  0e10
//
//struct nlist
//{
//    uint32_t n_strx;    /* index into the string table */
//    uint8_t  n_type;    /* type flag, see below */
//    uint8_t  n_sect;    /* section number or NO_SECT */
//    int16_t  n_desc;    /* see <mach-o/stab.h> */
//    uint32_t n_value;   /* value of this symbol (or stab offset) */
//};
//
//struct nlist_64
//{
//    uint32_t n_strx;    /* index into the string table */
//    uint8_t  n_type;    /* type flag, see below */
//    uint8_t  n_sect;    /* section number or NO_SECT */
//    uint16_t n_desc;    /* see <mach-o/stab.h> */
//    uint64_t n_value;   /* value of this symbol (or stab offset) */
//};
//
///*
// * The n_type field really contains four fields:
// *  unsigned char   N_STAB:3,
// *                  N_PEXT:1,
// *                  N_TYPE:3,
// *                  N_EXT:1;
// * which are used via the following masks.
// */
//#define N_STAB      0xe0    /* if any of these bits set, a symbolic debugging entry */
//#define N_PEXT      0x10    /* private external symbol bit */
//#define N_TYPE      0x0e    /* mask for the type bits */
//#define N_EXT       0x01    /* external symbol bit, set for external symbols */
//
//#define N_UNDF      0x00    /* undefined, n_sect == NO_SECT */
//#define N_ABS       0x02    /* absolute, n_sect == NO_SECT */
//#define N_SECT      0x0e    /* defined in section number n_sect */
//#define N_PBUD      0x0c    /* prebound undefined (defined in a dylib) */
//#define N_INDR      0x0a    /* indirect */
//
//#define NO_SECT     0       /* symbol is not in any section */
//#define MAX_SECT    255     /* 1 thru 255 inclusive */
//
///*
// * Symbolic debugger symbols.  The comments give the conventional use for
// * 
// *  .stabs "n_name", n_type, n_sect, n_desc, n_value
// *
// * where n_type is the defined constant and not listed in the comment.  Other
// * fields not listed are zero. n_sect is the section ordinal the entry is
// * refering to.
// */
//#define N_GSYM      0x20    /* global symbol: name,,NO_SECT,type,0 */
//#define N_FNAME     0x22    /* procedure name (f77 kludge): name,,NO_SECT,0,0 */
//#define N_FUN       0x24    /* procedure: name,,n_sect,linenumber,address */
//#define N_STSYM     0x26    /* static symbol: name,,n_sect,type,address */
//#define N_LCSYM     0x28    /* .lcomm symbol: name,,n_sect,type,address */
//#define N_BNSYM     0x2e    /* begin nsect sym: 0,,n_sect,0,address */
//#define N_AST       0x32    /* AST file path: name,,NO_SECT,0,0 */
//#define N_OPT       0x3c    /* emitted with gcc2_compiled and in gcc source */
//#define N_RSYM      0x40    /* register sym: name,,NO_SECT,type,register */
//#define N_SLINE     0x44    /* src line: 0,,n_sect,linenumber,address */
//#define N_ENSYM     0x4e    /* end nsect sym: 0,,n_sect,0,address */
//#define N_SSYM      0x60    /* structure elt: name,,NO_SECT,type,struct_offset */
//#define N_SO        0x64    /* source file name: name,,n_sect,0,address */
//#define N_OSO       0x66    /* object file name: name,,(see below),0,st_mtime */
//#define N_LSYM      0x80    /* local sym: name,,NO_SECT,type,offset */
//#define N_BINCL     0x82    /* include file beginning: name,,NO_SECT,0,sum */
//#define N_SOL       0x84    /* #included file name: name,,n_sect,0,address */
//#define N_PARAMS    0x86    /* compiler parameters: name,,NO_SECT,0,0 */
//#define N_VERSION   0x88    /* compiler version: name,,NO_SECT,0,0 */
//#define N_OLEVEL    0x8A    /* compiler -O level: name,,NO_SECT,0,0 */
//#define N_PSYM      0xa0    /* parameter: name,,NO_SECT,type,offset */
//#define N_EINCL     0xa2    /* include file end: name,,NO_SECT,0,0 */
//#define N_ENTRY     0xa4    /* alternate entry: name,,n_sect,linenumber,address */
//#define N_LBRAC     0xc0    /* left bracket: 0,,NO_SECT,nesting level,address */
//#define N_EXCL      0xc2    /* deleted include file: name,,NO_SECT,0,sum */
//#define N_RBRAC     0xe0    /* right bracket: 0,,NO_SECT,nesting level,address */
//#define N_BCOMM     0xe2    /* begin common: name,,NO_SECT,0,0 */
//#define N_ECOMM     0xe4    /* end common: name,,n_sect,0,0 */
//#define N_ECOML     0xe8    /* end common (local name): 0,,n_sect,0,address */
//#define N_LENG      0xfe    /* second stab entry with length information */

#endif // _MACHO_H
