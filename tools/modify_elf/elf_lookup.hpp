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


#include <array>
#include <regex>


template <class T>
struct argument
    : public std::tuple<bool, T>
{
    constexpr argument()
        : std::tuple<bool, T> { false, T {} }
    {}

    template <class... Args>
    constexpr argument(Args&&... args)
        : std::tuple<bool, T> { std::forward<Args>(args)... }
    {}

    operator bool() const
    {
        return std::get<0>(*this);
    }

    operator const T&() const
    {
        return value();
    }

    const T& value() const
    {
        return std::get<1>(*this);
    }
};



template <class T, class Tuple, size_t... Is>
auto make_shared_from_tuple(Tuple&& tuple, std::index_sequence<Is...> )
{
    return std::make_shared<T>(std::get<Is>(std::forward<Tuple>(tuple))...);
}

template <class T, class Tuple>
auto make_shared_from_tuple(Tuple&& tuple)
{
    return make_shared_from_tuple<T>(
        std::forward<Tuple>(tuple),
        std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>{}
    );
}

template <class T, class Tuple, size_t... Is>
auto construct_from_tuple(Tuple&& tuple, std::index_sequence<Is...> )
{
    return T { std::get<Is>(std::forward<Tuple>(tuple))... };
}

template <class T, class Tuple>
auto construct_from_tuple(Tuple&& tuple)
{
    return construct_from_tuple<T>(
        std::forward<Tuple>(tuple),
        std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>{}
    );
}



const std::regex re_integer { "^(?:0x([0-9A-Fa-f]+)|0([0-7]+)|(0|[1-9][0-9]*))$" };

template <class T>
T parse_int(std::string_view value)
{
    std::cmatch match;
    if (std::regex_match(value.begin(), value.end(), match, re_integer)) {
        if (match[1].matched) {
            return static_cast<T>(std::stoull(match[1].str(), nullptr, 16));
        } else if (match[2].matched) {
            return static_cast<T>(std::stoull(match[2].str(), nullptr, 8));
        } else if (match[3].matched) {
            return static_cast<T>(std::stoull(match[3].str(), nullptr, 10));
        }
    }
    throw std::invalid_argument { "cannot parse string to integer" };
}





template <class T>
struct integer_parser
{
    using value_type = T;
    using result = argument<T>;

    std::string_view string;
    bool is_base;
    T value;

    result match(const std::string_view flag) const
    {
        if (is_base) {
            if (flag.starts_with(string)) {
                auto rest = flag.substr(string.size());
                return { true, value + parse_int<T>(rest) };
            }
        } else if (flag == string) {
            return { true, value };
        }
        return { false, 0 };
    }
};




/*
auto parse_fields(std::string_view input, auto parser)
{
    return parser.
    return 0;
}
*/


auto split_string_view(std::string_view input, auto sep)
{
    std::vector<std::string_view> result;
    std::string_view::size_type l = 0, i;

    while ((i = input.find(sep, l)) != std::string_view::npos) {
        result.emplace_back(input.substr(l, i-l));
        l = i + 1;
    }
    result.emplace_back(input.substr(l));

    return result;
}



template <class... Parsers>
struct sequence_parser;

template <class Parser, class... Rest>
struct sequence_parser<Parser, Rest...>
{
    using next_type = sequence_parser<Rest...>;
    using this_value_type = typename std::remove_cvref_t<Parser>::value_type;
    using this_default_type = typename std::remove_cvref_t<Parser>::default_type;
    using value_type = decltype(std::tuple_cat(
        std::declval<std::tuple<this_value_type>>(),
        std::declval<typename next_type::value_type>()
    ));
    using default_type = decltype(std::tuple_cat(
        std::declval<std::tuple<this_default_type>>(),
        std::declval<typename next_type::default_type>()
    ));
    static constexpr auto npos = std::string_view::npos;

    next_type next;
    Parser parser;
    std::string_view sep;

    constexpr sequence_parser(Parser&& parser, Rest&&... rest,
                              std::string_view sep = ",")
        : parser { std::forward<Parser>(parser) }
        , next { std::forward<Rest>(rest)..., sep }
        , sep { sep }
    {}

    value_type match(std::string_view input) const
    {
        auto pos = input.find(sep);
        if constexpr (std::tuple_size<value_type>() == 1) {
            if (pos == npos) {
                return std::make_tuple(parser.match(input));
            } else {
                throw std::invalid_argument { "too many items in sequence" };
            }
        } else {
            if (pos == npos) {
                throw std::invalid_argument { "too few items in sequence" };
            } else {
                return std::tuple_cat(
                    std::make_tuple(parser.match(input.substr(0, pos))),
                    next.match(input.substr(pos+1))
                );
            }
        }
    }
};

template <>
struct sequence_parser<>
{
    using value_type = std::tuple<>;
    using default_type = std::tuple<>;

    constexpr sequence_parser(std::string_view sep = ",")
    {}
};



template <class T, class... Parsers>
struct instance_parser
    : public sequence_parser<Parsers...>
{
    using base = sequence_parser<Parsers...>;
    using value_type = T;
    using default_type = value_type;

    constexpr instance_parser(Parsers&&... parsers)
        : base { std::forward<Parsers>(parsers)... }
    {}

    value_type match(std::string_view input) const
    {
        return construct_from_tuple<value_type>(base::match(input));
    }
};

template <class T, class... Parsers>
struct instance_parser<std::shared_ptr<T>, Parsers...>
    : public sequence_parser<Parsers...>
{
    using base = sequence_parser<Parsers...>;
    using value_type = std::shared_ptr<T>;
    using default_type = value_type;

    constexpr instance_parser(Parsers&&... parsers)
        : base { std::forward<Parsers>(parsers)... }
    {}

    value_type match(std::string_view input) const
    {
        return make_shared_from_tuple<T>(base::match(input));
    }
};

template <class T, class... Parsers>
constexpr auto make_instance_parser(Parsers&&... parsers)
{
    return instance_parser<T, Parsers...> {
        std::forward<Parsers>(parsers)...
    };
}


template <class T, class... Parsers>
struct choice_parser;

template <class T, class Parser, class... Rest>
struct choice_parser<T, Parser, Rest...>
{
    using value_type = T;
    using default_type = value_type;

    choice_parser<T, Rest...> next;
    Parser parser;

    constexpr choice_parser(Parser&& parser, Rest&&... rest)
        : next { std::forward<Rest>(rest)... }
        , parser { std::forward<Parser>(parser) }
    {}

    value_type match(std::string_view input) const
    {
        try {
            return static_cast<value_type>(parser.match(input));
        } catch (std::invalid_argument&) {}
        return next.match(input);
    }
};

template <class T>
struct choice_parser<T>
{
    using value_type = T;
    using default_type = value_type;

    constexpr choice_parser()
    {}

    value_type match(std::string_view input) const
    {
        throw std::invalid_argument { "" };
    }
};

template <class T, class... Parsers>
constexpr auto make_choice_parser(Parsers&&... parsers)
{
    return choice_parser<T, Parsers...> {
        std::forward<Parsers>(parsers)...
    };
}


template <class Parser>
struct default_value_parser
{
    using value_type = typename Parser::value_type;
    using default_type = typename Parser::default_type;

    const Parser& parser;
    default_type value;
    std::string_view to_match;

    constexpr default_value_parser(const Parser& parser, default_type&& value,
                                   std::string_view to_match = "")
        : parser { parser }
        , value { std::forward<default_type>(value) }
        , to_match { to_match }
    {}

    constexpr default_value_parser(const Parser& parser,
                                   const default_type& value,
                                   std::string_view to_match = "")
        : parser { parser }
        , value { value }
        , to_match { to_match }
    {}

    value_type match(const std::string_view input) const
    {
        return input == to_match ? value_type { value } : parser.match(input);
    }
};


template <class T, class D = T>
struct value_parser;


template <>
struct value_parser<std::string, std::string_view>
{
    using value_type = std::string;
    using default_type = std::string_view;

    std::string_view pattern;
    bool allows_empty;

    constexpr value_parser(bool allows_empty = true)
        : pattern {}
        , allows_empty { allows_empty }
    {}

    constexpr value_parser(const char* pattern)
        : pattern { pattern }
        , allows_empty { false }
    {}

    value_type match(const std::string_view input) const
    {
        if (!pattern.empty()) {
            std::regex regex { pattern.begin(), pattern.end() };
            std::cmatch match;
            if (std::regex_match(input.begin(), input.end(), match, regex)) {
                auto group = match.size() > 1 ? match[1] : match[0];
                return group.str();
            } else {
                throw std::invalid_argument { "" };
            }
        } else if (!allows_empty && input.empty()) {
            throw std::invalid_argument { "" };
        } else {
            return std::string { input };
        }
    }

    constexpr auto with_default(default_type&& value, std::string_view to_match = "") const
    {
        return default_value_parser {
            *this, std::forward<default_type>(value), to_match
        };
    }

    constexpr auto with_default(const default_type& value, std::string_view to_match = "") const
    {
        return default_value_parser { *this, value, to_match };
    }
};


template <std::integral I>
struct value_parser<I, I>
{
protected:
    static constexpr std::string_view prune_star(std::string_view s)
    {
        return s.ends_with('*') ? s.substr(0, s.size() - 1) : s;
    }

public:
    using value_type = I;
    using default_type = value_type;

    std::string_view string;
    bool is_base;
    I value;

    constexpr value_parser(std::string_view string = "", I value = {})
        : is_base { string.ends_with('*') }
        , string { prune_star(string) }
        , value { value }
    {}

    value_type match(const std::string_view input) const
    {
        if (string.empty()) {
            return parse_int<I>(input);
        } else if (is_base) {
            if (input.starts_with(string)) {
                auto rest = input.substr(string.size());
                return value + parse_int<I>(rest);
            }
        } else if (input == string) {
            return value;
        }
        throw std::invalid_argument { "" };
    }

    constexpr auto with_default(default_type&& value, std::string_view to_match = "") const
    {
        return default_value_parser {
            *this, std::forward<default_type>(value), to_match
        };
    }

    constexpr auto with_default(const default_type& value, std::string_view to_match = "") const
    {
        return default_value_parser { *this, value, to_match };
    }
};




template <class T, class D, size_t N>
struct parser_group
{
    using parser_type = value_parser<T, D>;
    using value_type = typename parser_type::value_type;
    using default_type = typename parser_type::default_type;

    std::string_view name;
    std::array<parser_type, N> parsers;

    constexpr parser_group(std::string_view name,
                           const parser_type (&parsers)[N])
        : name { name }
        , parsers { std::to_array(parsers) }
    {}

    value_type match(std::string_view input) const
    {
        for (auto&& parser : parsers) {
            try {
                return parser.match(input);
            } catch (std::invalid_argument&) {}
        }
        throw std::invalid_argument { "fixme" };
    }

    constexpr auto with_default(default_type&& value, std::string_view to_match = "") const
    {
        return default_value_parser {
            *this, std::forward<default_type>(value), to_match
        };
    }

    constexpr auto with_default(const default_type& value, std::string_view to_match = "") const
    {
        return default_value_parser { *this, value, to_match };
    }
};

template <class T, size_t N, class D = T>
constexpr auto make_parser_group(std::string_view name,
                                 const value_parser<T, D> (&parsers)[N])
{
    return parser_group<T, D, N> { name, parsers };
}










namespace lookup {
namespace detail {

static constexpr integer_parser<Elf_Half> elf_type[] = {
    { "int:",               true,   0                       },
    { "os+",                true,   ET_LOOS                 },
    { "proc+",              true,   ET_LOPROC               },
    { "core",               false,  ET_CORE                 },
    { "dyn",                false,  ET_DYN                  },
    { "exec",               false,  ET_EXEC                 },
    { "none",               false,  ET_NONE                 },
    { "rel",                false,  ET_REL                  },
};

static constexpr integer_parser<Elf_Half> elf_machine[] = {
    { "int:",               true,   0                       },
    { "386",                false,  EM_386                  },
    { "486",                false,  EM_486                  },
    { "56800ex",            false,  EM_56800EX              },
    { "65816",              false,  EM_65816                },
    { "68hc05",             false,  EM_68HC05               },
    { "68hc08",             false,  EM_68HC08               },
    { "68hc11",             false,  EM_68HC11               },
    { "68hc12",             false,  EM_68HC12               },
    { "68hc16",             false,  EM_68HC16               },
    { "68k",                false,  EM_68K                  },
    { "78kor",              false,  EM_78KOR                },
    { "8051",               false,  EM_8051                 },
    { "860",                false,  EM_860                  },
    { "88k",                false,  EM_88K                  },
    { "960",                false,  EM_960                  },
    { "aarch64",            false,  EM_AARCH64              },
    { "adapteva_epiphany",  false,  EM_ADAPTEVA_EPIPHANY    },
    { "alpha",              false,  EM_ALPHA                },
    { "altera_nios2",       false,  EM_ALTERA_NIOS2         },
    { "amdgpu",             false,  EM_AMDGPU               },
    { "arc",                false,  EM_ARC                  },
    { "arc_a5",             false,  EM_ARC_A5               },
    { "arc_compact2",       false,  EM_ARC_COMPACT2         },
    { "arc_compact3",       false,  EM_ARC_COMPACT3         },
    { "arc_compact3_64",    false,  EM_ARC_COMPACT3_64      },
    { "arca",               false,  EM_ARCA                 },
    { "arm",                false,  EM_ARM                  },
    { "avr",                false,  EM_AVR                  },
    { "avr32",              false,  EM_AVR32                },
    { "ba1",                false,  EM_BA1                  },
    { "ba2",                false,  EM_BA2                  },
    { "blackfin",           false,  EM_BLACKFIN             },
    { "bpf",                false,  EM_BPF                  },
    { "c166",               false,  EM_C166                 },
    { "cdp",                false,  EM_CDP                  },
    { "ce",                 false,  EM_CE                   },
    { "ceva",               false,  EM_CEVA                 },
    { "ceva_x2",            false,  EM_CEVA_X2              },
    { "cloudshield",        false,  EM_CLOUDSHIELD          },
    { "coge",               false,  EM_COGE                 },
    { "coldfire",           false,  EM_COLDFIRE             },
    { "cool",               false,  EM_COOL                 },
    { "corea_1st",          false,  EM_COREA_1ST            },
    { "corea_2nd",          false,  EM_COREA_2ND            },
    { "cr",                 false,  EM_CR                   },
    { "cr16",               false,  EM_CR16                 },
    { "craynv2",            false,  EM_CRAYNV2              },
    { "cris",               false,  EM_CRIS                 },
    { "crx",                false,  EM_CRX                  },
    { "csky",               false,  EM_CSKY                 },
    { "csr_kalimba",        false,  EM_CSR_KALIMBA          },
    { "cuda",               false,  EM_CUDA                 },
    { "cygnus_frv",         false,  EM_CYGNUS_FRV           },
    { "cygnus_mep",         false,  EM_CYGNUS_MEP           },
    { "cypress_m8c",        false,  EM_CYPRESS_M8C          },
    { "d10v",               false,  EM_D10V                 },
    { "d30v",               false,  EM_D30V                 },
    { "dlx",                false,  EM_DLX                  },
    { "dsp24",              false,  EM_DSP24                },
    { "dspic30f",           false,  EM_DSPIC30F             },
    { "dxp",                false,  EM_DXP                  },
    { "ecog1",              false,  EM_ECOG1                },
    { "ecog16",             false,  EM_ECOG16               },
    { "ecog1x",             false,  EM_ECOG1X               },
    { "ecog2",              false,  EM_ECOG2                },
    { "etpu",               false,  EM_ETPU                 },
    { "excess",             false,  EM_EXCESS               },
    { "f2mc16",             false,  EM_F2MC16               },
    { "firepath",           false,  EM_FIREPATH             },
    { "fr20",               false,  EM_FR20                 },
    { "fr30",               false,  EM_FR30                 },
    { "ft32",               false,  EM_FT32                 },
    { "fx66",               false,  EM_FX66                 },
    { "graphcore_ipu",      false,  EM_GRAPHCORE_IPU        },
    { "h8_300",             false,  EM_H8_300               },
    { "h8_300h",            false,  EM_H8_300H              },
    { "h8_500",             false,  EM_H8_500               },
    { "h8s",                false,  EM_H8S                  },
    { "huany",              false,  EM_HUANY                },
    { "ia_64",              false,  EM_IA_64                },
    { "img1",               false,  EM_IMG1                 },
    { "ip2k",               false,  EM_IP2K                 },
    { "iq2000",             false,  EM_IQ2000               },
    { "javelin",            false,  EM_JAVELIN              },
    { "kf32",               false,  EM_KF32                 },
    { "km32",               false,  EM_KM32                 },
    { "kmx16",              false,  EM_KMX16                },
    { "kmx32",              false,  EM_KMX32                },
    { "kmx8",               false,  EM_KMX8                 },
    { "kvarc",              false,  EM_KVARC                },
    { "kvx",                false,  EM_KVX                  },
    { "l1om",               false,  EM_L1OM                 },
    { "lanai",              false,  EM_LANAI                },
    { "latticemico32",      false,  EM_LATTICEMICO32        },
    { "loongarch",          false,  EM_LOONGARCH            },
    { "m16c",               false,  EM_M16C                 },
    { "m32",                false,  EM_M32                  },
    { "m32c",               false,  EM_M32C                 },
    { "m32c_old",           false,  EM_M32C_OLD             },
    { "m32r",               false,  EM_M32R                 },
    { "manik",              false,  EM_MANIK                },
    { "max",                false,  EM_MAX                  },
    { "maxq30",             false,  EM_MAXQ30               },
    { "mchp_pic",           false,  EM_MCHP_PIC             },
    { "mcore",              false,  EM_MCORE                },
    { "mcs6502",            false,  EM_MCS6502              },
    { "mcst_elbrus",        false,  EM_MCST_ELBRUS          },
    { "me16",               false,  EM_ME16                 },
    { "metag",              false,  EM_METAG                },
    { "microblaze",         false,  EM_MICROBLAZE           },
    { "mips",               false,  EM_MIPS                 },
    { "mips_rs3_le",        false,  EM_MIPS_RS3_LE          },
    { "mips_x",             false,  EM_MIPS_X               },
    { "mma",                false,  EM_MMA                  },
    { "mmdsp_plus",         false,  EM_MMDSP_PLUS           },
    { "mmix",               false,  EM_MMIX                 },
    { "mn10200",            false,  EM_MN10200              },
    { "mn10300",            false,  EM_MN10300              },
    { "moxie",              false,  EM_MOXIE                },
    { "msp430",             false,  EM_MSP430               },
    { "mt",                 false,  EM_MT                   },
    { "ncpu",               false,  EM_NCPU                 },
    { "ndr1",               false,  EM_NDR1                 },
    { "nds32",              false,  EM_NDS32                },
    { "nfp",                false,  EM_NFP                  },
    { "nios32",             false,  EM_NIOS32               },
    { "none",               false,  EM_NONE                 },
    { "norc",               false,  EM_NORC                 },
    { "ns32k",              false,  EM_NS32K                },
    { "old_alpha",          false,  EM_OLD_ALPHA            },
    { "open8",              false,  EM_OPEN8                },
    { "openrisc",           false,  EM_OPENRISC             },
    { "parisc",             false,  EM_PARISC               },
    { "pcp",                false,  EM_PCP                  },
    { "pdp10",              false,  EM_PDP10                },
    { "pdp11",              false,  EM_PDP11                },
    { "pdsp",               false,  EM_PDSP                 },
    { "pj",                 false,  EM_PJ                   },
    { "ppc",                false,  EM_PPC                  },
    { "ppc64",              false,  EM_PPC64                },
    { "prism",              false,  EM_PRISM                },
    { "qdsp6",              false,  EM_QDSP6                },
    { "r32c",               false,  EM_R32C                 },
    { "rce",                false,  EM_RCE                  },
    { "rh32",               false,  EM_RH32                 },
    { "riscv",              false,  EM_RISCV                },
    { "rl78",               false,  EM_RL78                 },
    { "rs08",               false,  EM_RS08                 },
    { "rx",                 false,  EM_RX                   },
    { "s12z",               false,  EM_S12Z                 },
    { "s370",               false,  EM_S370                 },
    { "s390",               false,  EM_S390                 },
    { "score",              false,  EM_SCORE                },
    { "score7",             false,  EM_SCORE7               },
    { "se_c17",             false,  EM_SE_C17               },
    { "se_c33",             false,  EM_SE_C33               },
    { "sep",                false,  EM_SEP                  },
    { "sh",                 false,  EM_SH                   },
    { "sle9x",              false,  EM_SLE9X                },
    { "snp1k",              false,  EM_SNP1K                },
    { "sparc",              false,  EM_SPARC                },
    { "sparc32plus",        false,  EM_SPARC32PLUS          },
    { "sparcv9",            false,  EM_SPARCV9              },
    { "spu",                false,  EM_SPU                  },
    { "st100",              false,  EM_ST100                },
    { "st19",               false,  EM_ST19                 },
    { "st200",              false,  EM_ST200                },
    { "st7",                false,  EM_ST7                  },
    { "st9plus",            false,  EM_ST9PLUS              },
    { "starcore",           false,  EM_STARCORE             },
    { "stm8",               false,  EM_STM8                 },
    { "stxp7x",             false,  EM_STXP7X               },
    { "svx",                false,  EM_SVX                  },
    { "ti_c2000",           false,  EM_TI_C2000             },
    { "ti_c5500",           false,  EM_TI_C5500             },
    { "ti_c6000",           false,  EM_TI_C6000             },
    { "tile64",             false,  EM_TILE64               },
    { "tilegx",             false,  EM_TILEGX               },
    { "tilepro",            false,  EM_TILEPRO              },
    { "tinyj",              false,  EM_TINYJ                },
    { "tmm_gpp",            false,  EM_TMM_GPP              },
    { "tpc",                false,  EM_TPC                  },
    { "tricore",            false,  EM_TRICORE              },
    { "trimedia",           false,  EM_TRIMEDIA             },
    { "tsk3000",            false,  EM_TSK3000              },
    { "unicore",            false,  EM_UNICORE              },
    { "v800",               false,  EM_V800                 },
    { "v850",               false,  EM_V850                 },
    { "vax",                false,  EM_VAX                  },
    { "videocore",          false,  EM_VIDEOCORE            },
    { "videocore3",         false,  EM_VIDEOCORE3           },
    { "videocore5",         false,  EM_VIDEOCORE5           },
    { "visium",             false,  EM_VISIUM               },
    { "vpp550",             false,  EM_VPP550               },
    { "webassembly",        false,  EM_WEBASSEMBLY          },
    { "x86_64",             false,  EM_X86_64               },
    { "xcore",              false,  EM_XCORE                },
    { "xgate",              false,  EM_XGATE                },
    { "ximo16",             false,  EM_XIMO16               },
    { "xstormy16",          false,  EM_XSTORMY16            },
    { "xtensa",             false,  EM_XTENSA               },
    { "z80",                false,  EM_Z80                  },
    { "zsp",                false,  EM_ZSP                  },
};

static constexpr integer_parser<unsigned char> elf_osabi[] = {
    { "int:",               true,   0                       },
    { "arch+",              true,   64                      },
    { "none",               false,  ELFOSABI_NONE           },
    { "sysv",               false,  ELFOSABI_NONE           },
    { "hpux",               false,  ELFOSABI_HPUX           },
    { "netbsd",             false,  ELFOSABI_NETBSD         },
    { "linux",              false,  ELFOSABI_LINUX          },
    { "hurd",               false,  ELFOSABI_HURD           },
    { "solaris",            false,  ELFOSABI_SOLARIS        },
    { "aix",                false,  ELFOSABI_AIX            },
    { "irix",               false,  ELFOSABI_IRIX           },
    { "freebsd",            false,  ELFOSABI_FREEBSD        },
    { "tru64",              false,  ELFOSABI_TRU64          },
    { "modesto",            false,  ELFOSABI_MODESTO        },
    { "openbsd",            false,  ELFOSABI_OPENBSD        },
    { "openvms",            false,  ELFOSABI_OPENVMS        },
    { "nsk",                false,  ELFOSABI_NSK            },
    { "aros",               false,  ELFOSABI_AROS           },
    { "fenixos",            false,  ELFOSABI_FENIXOS        },
    { "nuxi",               false,  ELFOSABI_NUXI           },
    { "openvos",            false,  ELFOSABI_OPENVOS        },
    { "arm",                false,  ELFOSABI_ARM            },
    { "standalone",         false,  ELFOSABI_STANDALONE     },
};

static constexpr integer_parser<unsigned char> elf_abiversion[] = {
    { "int:",               true,   0                       },
    { "amdgpu_hsa",         false,  ELFOSABI_AMDGPU_HSA     },
    { "amdgpu_pal",         false,  ELFOSABI_AMDGPU_PAL     },
    { "amdgpu_mesa3d",      false,  ELFOSABI_AMDGPU_MESA3D  },
};

static constexpr integer_parser<Elf_Word> elf_flag[] = {
    { "int:",               true,   0                       },
};

static constexpr integer_parser<Elf_Word> elf_section_name[] = {
    { "int:",               true,   0                       },
    { "os+",                true,   SHN_LOOS                },
    { "proc+",              true,   SHN_LOPROC              },
    { "reserve+",           true,   SHN_LORESERVE           },
    { "abs",                false,  SHN_ABS                 },
    { "common",             false,  SHN_COMMON              },
    { "undef",              false,  SHN_UNDEF               },
    { "xindex",             false,  SHN_XINDEX              },
};

static constexpr integer_parser<Elf_Word> elf_section_type[] = {
    { "int:",               true,   0                       },
    { "os+",                true,   SHT_LOOS                },
    { "proc+",              true,   SHT_LOPROC              },
    { "user+",              true,   SHT_LOUSER              },
    { "checksum",           false,  SHT_CHECKSUM            },
    { "dynamic",            false,  SHT_DYNAMIC             },
    { "dynsym",             false,  SHT_DYNSYM              },
    { "fini_array",         false,  SHT_FINI_ARRAY          },
    { "group",              false,  SHT_GROUP               },
    { "hash",               false,  SHT_HASH                },
    { "init_array",         false,  SHT_INIT_ARRAY          },
    { "nobits",             false,  SHT_NOBITS              },
    { "note",               false,  SHT_NOTE                },
    { "null",               false,  SHT_NULL                },
    { "preinit_array",      false,  SHT_PREINIT_ARRAY       },
    { "progbits",           false,  SHT_PROGBITS            },
    { "rel",                false,  SHT_REL                 },
    { "rela",               false,  SHT_RELA                },
    { "shlib",              false,  SHT_SHLIB               },
    { "strtab",             false,  SHT_STRTAB              },
    { "symtab",             false,  SHT_SYMTAB              },
    { "symtab_shndx",       false,  SHT_SYMTAB_SHNDX        },
    { "sunw+",              true,   SHT_LOSUNW              },
    { "sunw_move",          false,  SHT_SUNW_move           },
    { "sunw_comdat",        false,  SHT_SUNW_COMDAT         },
    { "sunw_syminfo",       false,  SHT_SUNW_syminfo        },
    { "gnu_attributes",     false,  SHT_GNU_ATTRIBUTES      },
    { "gnu_hash",           false,  SHT_GNU_HASH            },
    { "gnu_liblist",        false,  SHT_GNU_LIBLIST         },
    { "gnu_verdef",         false,  SHT_GNU_verdef          },
    { "gnu_verneed",        false,  SHT_GNU_verneed         },
    { "gnu_versym",         false,  SHT_GNU_versym          },
    { "arm_exidx",          false,  SHT_ARM_EXIDX           },
    { "arm_preemptmap",     false,  SHT_ARM_PREEMPTMAP      },
    { "arm_attributes",     false,  SHT_ARM_ATTRIBUTES      },
    { "arm_debugoverlay",   false,  SHT_ARM_DEBUGOVERLAY    },
    { "arm_overlaysection", false,  SHT_ARM_OVERLAYSECTION  },
    { "rpl_exports",        false,  SHT_RPL_EXPORTS         },
    { "rpl_imports",        false,  SHT_RPL_IMPORTS         },
    { "rpl_crcs",           false,  SHT_RPL_CRCS            },
    { "rpl_fileinfo",       false,  SHT_RPL_FILEINFO        },
};

static constexpr integer_parser<Elf_Xword> elf_section_flag[] = {
    { "int:",               true,   0                       },
    { "alloc",              false,  SHF_ALLOC               },
    { "compressed",         false,  SHF_COMPRESSED          },
    { "exclude",            false,  SHF_EXCLUDE             },
    { "execinstr",          false,  SHF_EXECINSTR           },
    { "gnu_mbind",          false,  SHF_GNU_MBIND           },
    { "gnu_retain",         false,  SHF_GNU_RETAIN          },
    { "group",              false,  SHF_GROUP               },
    { "info_link",          false,  SHF_INFO_LINK           },
    { "link_order",         false,  SHF_LINK_ORDER          },
    { "merge",              false,  SHF_MERGE               },
    { "mips_gprel",         false,  SHF_MIPS_GPREL          },
    { "ordered",            false,  SHF_ORDERED             },
    { "os_nonconforming",   false,  SHF_OS_NONCONFORMING    },
    { "strings",            false,  SHF_STRINGS             },
    { "tls",                false,  SHF_TLS                 },
    { "write",              false,  SHF_WRITE               },
    { "rpx_deflate",        false,  SHF_RPX_DEFLATE         },
};

static constexpr integer_parser<Elf_Word> elf_section_group[] = {
    { "int:",               true,   0                       },
    { "comdat",             false,  GRP_COMDAT              },
};

static constexpr integer_parser<unsigned char> elf_symbol_binding[] = {
    { "int:",               true,   0                       },
    { "os+",                true,   STB_LOOS                },
    { "proc+",              true,   STB_LOPROC              },
    { "global",             false,  STB_GLOBAL              },
    { "local",              false,  STB_LOCAL               },
    { "multidef",           false,  STB_MULTIDEF            },
    { "weak",               false,  STB_WEAK                },
};

static constexpr integer_parser<Elf_Word> elf_symbol_type[] = {
    { "int:",               true,   0                       },
    { "os+",                true,   STT_LOOS                },
    { "proc+",              true,   STT_LOPROC              },
    { "notype",             false,  STT_NOTYPE              },
    { "object",             false,  STT_OBJECT              },
    { "func",               false,  STT_FUNC                },
    { "section",            false,  STT_SECTION             },
    { "file",               false,  STT_FILE                },
    { "common",             false,  STT_COMMON              },
    { "tls",                false,  STT_TLS                 },
    { "amdgpu_hsa_kernel",  false,  STT_AMDGPU_HSA_KERNEL   },
};

static constexpr integer_parser<unsigned char> elf_symbol_visibility[] = {
    { "int:",               true,   0                       },
    { "default",            false,  STV_DEFAULT             },
    { "internal",           false,  STV_INTERNAL            },
    { "hidden",             false,  STV_HIDDEN              },
    { "protected",          false,  STV_PROTECTED           },
};

static constexpr integer_parser<Elf_Word> elf_symbol_name[] = {
    { "int:",               true,   0                       },
    { "undef",              false,  STN_UNDEF               },
};

} // ::detail

namespace elf {
    constexpr auto type = make_parser_group<Elf_Half>(
        "ELF type", {
        { "int:*",              0                       },
        { "os+*",               ET_LOOS                 },
        { "proc+*",             ET_LOPROC               },
        { "core",               ET_CORE                 },
        { "dyn",                ET_DYN                  },
        { "exec",               ET_EXEC                 },
        { "none",               ET_NONE                 },
        { "rel",                ET_REL                  },
    });

    //constexpr auto type = std::to_array(detail::elf_type);
    constexpr auto machine = std::to_array(detail::elf_machine);
    constexpr auto osabi = std::to_array(detail::elf_osabi);
    constexpr auto abiversion = std::to_array(detail::elf_abiversion);
    constexpr auto flag = std::to_array(detail::elf_flag);
    namespace section {
        constexpr auto name = std::to_array(detail::elf_section_name);
        constexpr auto type = std::to_array(detail::elf_section_type);
        constexpr auto flag = std::to_array(detail::elf_section_flag);
        constexpr auto group = std::to_array(detail::elf_section_group);
    } // ::section
    namespace symbol {
        constexpr auto name = value_parser<std::string, std::string_view> { false };
        constexpr auto name_index = make_parser_group<Elf_Word>(
            "symbol name index", {
            { "*",                  0                       },
            { "-",                  STN_UNDEF               },
        });
        constexpr auto section = value_parser<std::string, std::string_view> { false };
        constexpr auto section_index = make_parser_group<Elf_Half>(
            "symbol name index", {
            { "*",                  0                       },
            { "-",                  SHN_UNDEF               },
        });
        constexpr auto value = value_parser<Elf64_Addr> {};
        constexpr auto size = value_parser<Elf_Xword> {};
        constexpr auto info = value_parser<unsigned char> {};

        constexpr auto binding = make_parser_group<unsigned char>(
            "symbol binding", {
            { "int:*",              0                       },
            { "os+*",               STB_LOOS                },
            { "proc+*",             STB_LOPROC              },
            { "global",             STB_GLOBAL              },
            { "local",              STB_LOCAL               },
            { "multidef",           STB_MULTIDEF            },
            { "weak",               STB_WEAK                },
        });

        constexpr auto type = make_parser_group<Elf_Word>(
            "symbol type", {
            { "int:*",              0                       },
            { "os+*",               STT_LOOS                },
            { "proc+*",             STT_LOPROC              },
            { "notype",             STT_NOTYPE              },
            { "object",             STT_OBJECT              },
            { "func",               STT_FUNC                },
            { "section",            STT_SECTION             },
            { "file",               STT_FILE                },
            { "common",             STT_COMMON              },
            { "tls",                STT_TLS                 },
            { "amdgpu_hsa_kernel",  STT_AMDGPU_HSA_KERNEL   },
        });

        constexpr auto visibility = make_parser_group<unsigned char>(
            "symbol visibility", {
            { "int:*",              0                       },
            { "default",            STV_DEFAULT             },
            { "internal",           STV_INTERNAL            },
            { "hidden",             STV_HIDDEN              },
            { "protected",          STV_PROTECTED           },
        });

        //constexpr auto name = std::to_array(detail::elf_symbol_name);
    } // ::symbol
} // ::elf

} // ::lookup

/*
--set-branding <value>
    => memcpy(&e_ident[EI_ABIVERSION], <value>, min(strlen(<value>), EI_NIDENT-EI_ABIVERSION))
        <string>            "FreeBSD"
        <hexbytes>          "bytes:46726565425344"
--set-type <value>
    => e_type = <value>
--set-mach <value>
    => e_machine = <value>
--set-osabi <value>
    => e_ident[EI_OSABI] = <value>
--set-abiversion <value>
    => e_ident[EI_ABIVERSION] = <value>

--enable-flag <value>
    => e_flags |= <value>
--disable-flag <value>
    => e_flags &= ~<value>
        <int>                       <int>
        amdgpu_*
            xnack                   EF_AMDGPU_XNACK
            xnack_v2                EF_AMDGPU_FEATURE_XNACK_V2
            trap_handler_v2         EF_AMDGPU_FEATURE_TRAP_HANDLER_V2
            xnack_v3                EF_AMDGPU_FEATURE_XNACK_V3
            sramecc_v3              EF_AMDGPU_FEATURE_SRAMECC_V3
            xnack_v4                EF_AMDGPU_FEATURE_XNACK_V4
            xnack_unsupported_v4    EF_AMDGPU_FEATURE_XNACK_UNSUPPORTED_V4
            xnack_any_v4            EF_AMDGPU_FEATURE_XNACK_ANY_V4
            xnack_off_v4            EF_AMDGPU_FEATURE_XNACK_OFF_V4
            xnack_on_v4             EF_AMDGPU_FEATURE_XNACK_ON_V4
            sramecc_v4              EF_AMDGPU_FEATURE_SRAMECC_V4
            sramecc_unsupported_v4  EF_AMDGPU_FEATURE_SRAMECC_UNSUPPORTED_V4
            sramecc_any_v4          EF_AMDGPU_FEATURE_SRAMECC_ANY_V4
            sramecc_off_v4          EF_AMDGPU_FEATURE_SRAMECC_OFF_V4
            sramecc_on_v4           EF_AMDGPU_FEATURE_SRAMECC_ON_V4
            mach                    EF_AMDGPU_MACH
            mach_*
                none                EF_AMDGPU_MACH_NONE
                r600+<int>          EF_AMDGPU_MACH_R600_FIRST + <int>
                r600_*
                    r600            EF_AMDGPU_MACH_R600_R600
                    r630            EF_AMDGPU_MACH_R600_R630
                    rs880           EF_AMDGPU_MACH_R600_RS880
                    rv670           EF_AMDGPU_MACH_R600_RV670
                    rv710           EF_AMDGPU_MACH_R600_RV710
                    rv730           EF_AMDGPU_MACH_R600_RV730
                    rv770           EF_AMDGPU_MACH_R600_RV770
                    cedar           EF_AMDGPU_MACH_R600_CEDAR
                    cypress         EF_AMDGPU_MACH_R600_CYPRESS
                    juniper         EF_AMDGPU_MACH_R600_JUNIPER
                    redwood         EF_AMDGPU_MACH_R600_REDWOOD
                    sumo            EF_AMDGPU_MACH_R600_SUMO
                    barts           EF_AMDGPU_MACH_R600_BARTS
                    caicos          EF_AMDGPU_MACH_R600_CAICOS
                    cayman          EF_AMDGPU_MACH_R600_CAYMAN
                    turks           EF_AMDGPU_MACH_R600_TURKS
                amdgcn+<int>        EF_AMDGPU_MACH_AMDGCN_FIRST + <int>
                amdgcn_*
                    gfx600          EF_AMDGPU_MACH_AMDGCN_GFX600
                    gfx601          EF_AMDGPU_MACH_AMDGCN_GFX601
                    gfx700          EF_AMDGPU_MACH_AMDGCN_GFX700
                    gfx701          EF_AMDGPU_MACH_AMDGCN_GFX701
                    gfx702          EF_AMDGPU_MACH_AMDGCN_GFX702
                    gfx703          EF_AMDGPU_MACH_AMDGCN_GFX703
                    gfx704          EF_AMDGPU_MACH_AMDGCN_GFX704
                    gfx801          EF_AMDGPU_MACH_AMDGCN_GFX801
                    gfx802          EF_AMDGPU_MACH_AMDGCN_GFX802
                    gfx803          EF_AMDGPU_MACH_AMDGCN_GFX803
                    gfx810          EF_AMDGPU_MACH_AMDGCN_GFX810
                    gfx900          EF_AMDGPU_MACH_AMDGCN_GFX900
                    gfx902          EF_AMDGPU_MACH_AMDGCN_GFX902
                    gfx904          EF_AMDGPU_MACH_AMDGCN_GFX904
                    gfx906          EF_AMDGPU_MACH_AMDGCN_GFX906
                    gfx908          EF_AMDGPU_MACH_AMDGCN_GFX908
                    gfx909          EF_AMDGPU_MACH_AMDGCN_GFX909
                    gfx90c          EF_AMDGPU_MACH_AMDGCN_GFX90C
                    gfx1010         EF_AMDGPU_MACH_AMDGCN_GFX1010
                    gfx1011         EF_AMDGPU_MACH_AMDGCN_GFX1011
                    gfx1012         EF_AMDGPU_MACH_AMDGCN_GFX1012
                    gfx1030         EF_AMDGPU_MACH_AMDGCN_GFX1030
                    gfx1031         EF_AMDGPU_MACH_AMDGCN_GFX1031
                    gfx1032         EF_AMDGPU_MACH_AMDGCN_GFX1032
                    gfx1033         EF_AMDGPU_MACH_AMDGCN_GFX1033
                    gfx602          EF_AMDGPU_MACH_AMDGCN_GFX602
                    gfx705          EF_AMDGPU_MACH_AMDGCN_GFX705
                    gfx805          EF_AMDGPU_MACH_AMDGCN_GFX805
                    gfx1034         EF_AMDGPU_MACH_AMDGCN_GFX1034
                    gfx90a          EF_AMDGPU_MACH_AMDGCN_GFX90A
                    gfx1013         EF_AMDGPU_MACH_AMDGCN_GFX1013

    section header name
        <int>           <int>
        <string>        (do string lookup to get index)
        os+<int>        SHN_LOOS + <int>
        proc+<int>      SHN_LOPROC + <int>
        reserve+<int>   SHN_LORESERVE + <int>
        abs             SHN_ABS
        common          SHN_COMMON
        undef           SHN_UNDEF
        xindex          SHN_XINDEX

    section header type
        <int>               <int>
        os+<int>            SHT_LOOS + <int>
        proc+<int>          SHT_LOPROC + <int>
        user+<int>          SHT_LOUSER + <int>
        checksum            SHT_CHECKSUM
        dynamic             SHT_DYNAMIC
        dynsym              SHT_DYNSYM
        fini_array          SHT_FINI_ARRAY
        group               SHT_GROUP
        hash                SHT_HASH
        init_array          SHT_INIT_ARRAY
        nobits              SHT_NOBITS
        note                SHT_NOTE
        null                SHT_NULL
        preinit_array       SHT_PREINIT_ARRAY
        progbits            SHT_PROGBITS
        rel                 SHT_REL
        rela                SHT_RELA
        shlib               SHT_SHLIB
        strtab              SHT_STRTAB
        symtab              SHT_SYMTAB
        symtab_shndx        SHT_SYMTAB_SHNDX
        sunw+<int>          SHT_LOSUNW + <int>
        sunw_*
            move            SHT_SUNW_move
            comdat          SHT_SUNW_COMDAT
            syminfo         SHT_SUNW_syminfo
        gnu_*
            attributes      SHT_GNU_ATTRIBUTES
            hash            SHT_GNU_HASH
            liblist         SHT_GNU_LIBLIST
            verdef          SHT_GNU_verdef
            verneed         SHT_GNU_verneed
            versym          SHT_GNU_versym
        arm_*
            exidx           SHT_ARM_EXIDX
            preemptmap      SHT_ARM_PREEMPTMAP
            attributes      SHT_ARM_ATTRIBUTES
            debugoverlay    SHT_ARM_DEBUGOVERLAY
            overlaysection  SHT_ARM_OVERLAYSECTION
        rpl_* // used by Nintendo Wii U
            exports         SHT_RPL_EXPORTS
            imports         SHT_RPL_IMPORTS
            crcs            SHT_RPL_CRCS
            fileinfo        SHT_RPL_FILEINFO

    section header flags
        <int>               <int>
        alloc               SHF_ALLOC
        compressed          SHF_COMPRESSED
        exclude             SHF_EXCLUDE
        execinstr           SHF_EXECINSTR
        gnu_mbind           SHF_GNU_MBIND
        gnu_retain          SHF_GNU_RETAIN
        group               SHF_GROUP
        info_link           SHF_INFO_LINK
        link_order          SHF_LINK_ORDER
        merge               SHF_MERGE
        mips_gprel          SHF_MIPS_GPREL
        ordered             SHF_ORDERED
        os_nonconforming    SHF_OS_NONCONFORMING
        strings             SHF_STRINGS
        tls                 SHF_TLS
        write               SHF_WRITE
        rpx_* // used by Nintendo RPX/RPL (Wii U?)
            deflate         SHF_RPX_DEFLATE

    section group
        comdat              GRP_COMDAT

    symbol binding
        <int>               <int>
        os+<int>            STB_LOOS + <int>
        proc+<int>          STB_LOPROC + <int>
        global              STB_GLOBAL
        local               STB_LOCAL
        multidef            STB_MULTIDEF
        weak                STB_WEAK

    symbol types
        <int>               <int>
        os+<int>            STT_LOOS + <int>
        proc+<int>          STT_LOPROC + <int>
        notype              STT_NOTYPE
        object              STT_OBJECT
        func                STT_FUNC
        section             STT_SECTION
        file                STT_FILE
        common              STT_COMMON
        tls                 STT_TLS
        amdgpu_*
            hsa_*
                kernel      STT_AMDGPU_HSA_KERNEL

    symbol visibility
        default             STV_DEFAULT
        internal            STV_INTERNAL
        hidden              STV_HIDDEN
        protected           STV_PROTECTED

    symbol name
        undef               STN_UNDEF

    segment types
        <int>               <int>
        os+<int>            PT_LOOS + <int>
        proc+<int>          PT_LOPROC + <int>
        dynamic             PT_DYNAMIC
        interp              PT_INTERP
        load                PT_LOAD
        note                PT_NOTE
        null                PT_NULL
        pax_flags           PT_PAX_FLAGS
        phdr                PT_PHDR
        shlib               PT_SHLIB
        tls                 PT_TLS
        gnu_*
            eh_frame        PT_GNU_EH_FRAME
            mbind_hi        PT_GNU_MBIND_HI
            mbind_lo        PT_GNU_MBIND_LO
            property        PT_GNU_PROPERTY
            relro           PT_GNU_RELRO
            stack           PT_GNU_STACK
        openbsd_*
            bootdata        PT_OPENBSD_BOOTDATA
            randomize       PT_OPENBSD_RANDOMIZE
            wxneeded        PT_OPENBSD_WXNEEDED
        sunw_*
            bss             PT_SUNWBSS
            stack           PT_SUNWSTACK

    segment flags
        <int>               <int>
        x                   PF_X
        w                   PF_W
        r                   PF_R

    dynamic array tags
        <int>               <int>
        os+<int>            DT_LOOS + <int>
        proc+<int>          DT_LOPROC + <int>
        addrrnghi           DT_ADDRRNGHI
        audit               DT_AUDIT
        bind_now            DT_BIND_NOW
        config              DT_CONFIG
        debug               DT_DEBUG
        depaudit            DT_DEPAUDIT
        encoding            DT_ENCODING
        fini                DT_FINI
        fini_array          DT_FINI_ARRAY
        fini_arraysz        DT_FINI_ARRAYSZ
        flags               DT_FLAGS
        flags_1             DT_FLAGS_1
        hash                DT_HASH
        init                DT_INIT
        init_array          DT_INIT_ARRAY
        init_arraysz        DT_INIT_ARRAYSZ
        jmprel              DT_JMPREL
        maxpostags          DT_MAXPOSTAGS
        movetab             DT_MOVETAB
        needed              DT_NEEDED
        null                DT_NULL
        pltgot              DT_PLTGOT
        pltpad              DT_PLTPAD
        pltrel              DT_PLTREL
        pltrelsz            DT_PLTRELSZ
        preinit_array       DT_PREINIT_ARRAY
        preinit_arraysz     DT_PREINIT_ARRAYSZ
        rel                 DT_REL
        rela                DT_RELA
        relacount           DT_RELACOUNT
        relaent             DT_RELAENT
        relasz              DT_RELASZ
        relcount            DT_RELCOUNT
        relent              DT_RELENT
        relsz               DT_RELSZ
        rpath               DT_RPATH
        runpath             DT_RUNPATH
        soname              DT_SONAME
        strsz               DT_STRSZ
        strtab              DT_STRTAB
        symbolic            DT_SYMBOLIC
        syment              DT_SYMENT
        syminfo             DT_SYMINFO
        symtab              DT_SYMTAB
        textrel             DT_TEXTREL
        tlsdesc_got         DT_TLSDESC_GOT
        tlsdesc_plt         DT_TLSDESC_PLT
        verdef              DT_VERDEF
        verdefnum           DT_VERDEFNUM
        verneed             DT_VERNEED
        verneednum          DT_VERNEEDNUM
        versym              DT_VERSYM
        gnu_*
            conflict        DT_GNU_CONFLICT
            hash            DT_GNU_HASH
            liblist         DT_GNU_LIBLIST

    dynamic array flags
        <int>               <int>
        bind_now            DF_BIND_NOW
        origin              DF_ORIGIN
        static_tls          DF_STATIC_TLS
        symbolic            DF_SYMBOLIC
        textrel             DF_TEXTREL





// Values of note segment descriptor types for core files
NT_PRSTATUS   = 1; // Contains copy of prstatus struct
NT_FPREGSET   = 2; // Contains copy of fpregset struct
NT_PRPSINFO   = 3; // Contains copy of prpsinfo struct
NT_TASKSTRUCT = 4; // Contains copy of task struct
NT_AUXV       = 6; // Contains copy of Elfxx_auxv_t
NT_SIGINFO    = 0x53494749; // Fields of siginfo_t.
NT_FILE       = 0x46494c45; // Description of mapped files.

// Note segments for core files on dir-style procfs systems.
NT_PSTATUS      = 10; // Has a struct pstatus
NT_FPREGS       = 12; // Has a struct fpregset
NT_PSINFO       = 13; // Has a struct psinfo
NT_LWPSTATUS    = 16; // Has a struct lwpstatus_t
NT_LWPSINFO     = 17; // Has a struct lwpsinfo_t
NT_WIN32PSTATUS = 18; // Has a struct win32_pstatus

// Note name must be "LINUX"    
NT_PRXFPREG             = 0x46e62b7f; // Contains a user_xfpregs_struct
NT_PPC_VMX              = 0x100;      // PowerPC Altivec/VMX registers
NT_PPC_VSX              = 0x102;      // PowerPC VSX registers
NT_PPC_TAR              = 0x103;      // PowerPC Target Address Register
NT_PPC_PPR              = 0x104;      // PowerPC Program Priority Register
NT_PPC_DSCR             = 0x105;      // PowerPC Data Stream Control Register
NT_PPC_EBB              = 0x106;      // PowerPC Event Based Branch Registers
NT_PPC_PMU              = 0x107;      // PowerPC Performance Monitor Registers
NT_PPC_TM_CGPR          = 0x108;      // PowerPC TM checkpointed GPR Registers
NT_PPC_TM_CFPR          = 0x109;      // PowerPC TM checkpointed FPR Registers
NT_PPC_TM_CVMX          = 0x10a;      // PowerPC TM checkpointed VMX Registers
NT_PPC_TM_CVSX          = 0x10b;      // PowerPC TM checkpointed VSX Registers
NT_PPC_TM_SPR           = 0x10c;      // PowerPC TM Special Purpose Registers
NT_PPC_TM_CTAR          = 0x10d;      // PowerPC TM checkpointed TAR
NT_PPC_TM_CPPR          = 0x10e;      // PowerPC TM checkpointed PPR
NT_PPC_TM_CDSCR         = 0x10f;      // PowerPC TM checkpointed Data SCR
NT_386_TLS              = 0x200;      // x86 TLS information
NT_386_IOPERM           = 0x201;      // x86 io permissions
NT_X86_XSTATE           = 0x202;      // x86 XSAVE extended state
NT_X86_CET              = 0x203;      // x86 CET state.
NT_S390_HIGH_GPRS       = 0x300;      // S/390 upper halves of GPRs
NT_S390_TIMER           = 0x301;      // S390 timer
NT_S390_TODCMP          = 0x302;      // S390 TOD clock comparator
NT_S390_TODPREG         = 0x303;      // S390 TOD programmable register
NT_S390_CTRS            = 0x304;      // S390 control registers
NT_S390_PREFIX          = 0x305;      // S390 prefix register
NT_S390_LAST_BREAK      = 0x306;      // S390 breaking event address
NT_S390_SYSTEM_CALL     = 0x307;      // S390 system call restart data
NT_S390_TDB             = 0x308;      // S390 transaction diagnostic block
NT_S390_VXRS_LOW        = 0x309;      // S390 vector registers 0-15 upper half
NT_S390_VXRS_HIGH       = 0x30a;      // S390 vector registers 16-31
NT_S390_GS_CB           = 0x30b;      // s390 guarded storage registers
NT_S390_GS_BC           = 0x30c;      // s390 guarded storage broadcast control block
NT_ARM_VFP              = 0x400;      // ARM VFP registers
NT_ARM_TLS              = 0x401;      // AArch TLS registers
NT_ARM_HW_BREAK         = 0x402;      // AArch hardware breakpoint registers
NT_ARM_HW_WATCH         = 0x403;      // AArch hardware watchpoint registers
NT_ARM_SVE              = 0x405;      // AArch SVE registers.
NT_ARM_PAC_MASK         = 0x406;      // AArch pointer authentication code masks
NT_ARM_PACA_KEYS        = 0x407;      // ARM pointer authentication address keys
NT_ARM_PACG_KEYS        = 0x408;      // ARM pointer authentication generic keys
NT_ARM_TAGGED_ADDR_CTRL = 0x409;      // AArch64 tagged address control (prctl())
NT_ARM_PAC_ENABLED_KEYS = 0x40a;      // AArch64 pointer authentication enabled keys (prctl())
NT_ARC_V2               = 0x600;      // ARC HS accumulator/extra registers.
NT_LARCH_CPUCFG         = 0xa00;      // LoongArch CPU config registers
NT_LARCH_CSR            = 0xa01;      // LoongArch Control State Registers
NT_LARCH_LSX            = 0xa02;      // LoongArch SIMD eXtension registers
NT_LARCH_LASX           = 0xa03;      // LoongArch Advanced SIMD eXtension registers
NT_RISCV_CSR            = 0x900;      // RISC-V Control and Status Registers

// Note name must be "CORE"
NT_LARCH_LBT = 0xa04; // LoongArch Binary Translation registers

// The range 0xff000000 to 0xffffffff is set aside for notes that don't originate from any
// particular operating system.
NT_GDB_TDESC = 0xff000000; // Contains copy of GDB's target description XML.
NT_MEMTAG    = 0xff000001; // Contains a copy of the memory tags.
// ARM-specific NT_MEMTAG types.
NT_MEMTAG_TYPE_AARCH_MTE = 0x400; // MTE memory tags for AArch64.

constexpr Elf_Word NT_STAPSDT = 3; // Note segment for SystemTap probes.

// Note name is "FreeBSD"
NT_FREEBSD_THRMISC            = 7;  // Thread miscellaneous info.
NT_FREEBSD_PROCSTAT_PROC      = 8;  // Procstat proc data.
NT_FREEBSD_PROCSTAT_FILES     = 9;  // Procstat files data.
NT_FREEBSD_PROCSTAT_VMMAP     = 10; // Procstat vmmap data.
NT_FREEBSD_PROCSTAT_GROUPS    = 11; // Procstat groups data.
NT_FREEBSD_PROCSTAT_UMASK     = 12; // Procstat umask data.
NT_FREEBSD_PROCSTAT_RLIMIT    = 13; // Procstat rlimit data.
NT_FREEBSD_PROCSTAT_OSREL     = 14; // Procstat osreldate data.
NT_FREEBSD_PROCSTAT_PSSTRINGS = 15; // Procstat ps_strings data.
NT_FREEBSD_PROCSTAT_AUXV      = 16; // Procstat auxv data.
NT_FREEBSD_PTLWPINFO          = 17; // Thread ptrace miscellaneous info.

// Note name must start with  "NetBSD-CORE"
NT_NETBSDCORE_PROCINFO  = 1;  // Has a struct procinfo
NT_NETBSDCORE_AUXV      = 2;  // Has auxv data
NT_NETBSDCORE_LWPSTATUS = 24; // Has LWPSTATUS data
NT_NETBSDCORE_FIRSTMACH = 32; // start of machdep note types

// Note name is "OpenBSD"
NT_OPENBSD_PROCINFO = 10;
NT_OPENBSD_AUXV     = 11;
NT_OPENBSD_REGS     = 20;
NT_OPENBSD_FPREGS   = 21;
NT_OPENBSD_XFPREGS  = 22;
NT_OPENBSD_WCOOKIE  = 23;

// Note name must start with "SPU"
NT_SPU = 1;

// Values of note segment descriptor types for object files
NT_VERSION    = 1; // Contains a version string.
NT_ARCH       = 2; // Contains an architecture string.
NT_GO_BUILDID = 4; // Contains GO buildid data.

// Values for notes in non-core files using name "GNU"
NT_GNU_ABI_TAG         = 1;
NT_GNU_HWCAP           = 2; // Used by ld.so and kernel vDSO.
NT_GNU_BUILD_ID        = 3; // Generated by ld --build-id.
NT_GNU_GOLD_VERSION    = 4; // Generated by gold.
NT_GNU_PROPERTY_TYPE_0 = 5; // Generated by gcc.

NT_GNU_BUILD_ATTRIBUTE_OPEN = 0x100;
NT_GNU_BUILD_ATTRIBUTE_FUNC = 0x101;

    // Relocation types
        // x86
        386_*
            16                              R_386_16
            32                              R_386_32
            32plt                           R_386_32PLT
            8                               R_386_8
            copy                            R_386_COPY
            glob_dat                        R_386_GLOB_DAT
            got32                           R_386_GOT32
            got32x                          R_386_GOT32X
            gotoff                          R_386_GOTOFF
            gotpc                           R_386_GOTPC
            irelative                       R_386_IRELATIVE
            jmp_slot                        R_386_JMP_SLOT
            none                            R_386_NONE
            pc16                            R_386_PC16
            pc32                            R_386_PC32
            pc8                             R_386_PC8
            plt32                           R_386_PLT32
            relative                        R_386_RELATIVE
            size32                          R_386_SIZE32
            tls_desc                        R_386_TLS_DESC
            tls_desc_call                   R_386_TLS_DESC_CALL
            tls_dtpmod32                    R_386_TLS_DTPMOD32
            tls_dtpoff32                    R_386_TLS_DTPOFF32
            tls_gd                          R_386_TLS_GD
            tls_gd_32                       R_386_TLS_GD_32
            tls_gd_call                     R_386_TLS_GD_CALL
            tls_gd_pop                      R_386_TLS_GD_POP
            tls_gd_push                     R_386_TLS_GD_PUSH
            tls_gotdesc                     R_386_TLS_GOTDESC
            tls_gotie                       R_386_TLS_GOTIE
            tls_ie                          R_386_TLS_IE
            tls_ie_32                       R_386_TLS_IE_32
            tls_ldm                         R_386_TLS_LDM
            tls_ldm_32                      R_386_TLS_LDM_32
            tls_ldm_call                    R_386_TLS_LDM_CALL
            tls_ldm_pop                     R_386_TLS_LDM_POP
            tls_ldm_push                    R_386_TLS_LDM_PUSH
            tls_ldo_32                      R_386_TLS_LDO_32
            tls_le                          R_386_TLS_LE
            tls_le_32                       R_386_TLS_LE_32
            tls_tpoff                       R_386_TLS_TPOFF
            tls_tpoff32                     R_386_TLS_TPOFF32
        amdgpu_*
            abs32                           R_AMDGPU_ABS32
            abs32_hi                        R_AMDGPU_ABS32_HI
            abs32_lo                        R_AMDGPU_ABS32_LO
            abs64                           R_AMDGPU_ABS64
            gotpcrel                        R_AMDGPU_GOTPCREL
            gotpcrel32_hi                   R_AMDGPU_GOTPCREL32_HI
            gotpcrel32_lo                   R_AMDGPU_GOTPCREL32_LO
            none                            R_AMDGPU_NONE
            rel32                           R_AMDGPU_REL32
            rel32_hi                        R_AMDGPU_REL32_HI
            rel32_lo                        R_AMDGPU_REL32_LO
            rel64                           R_AMDGPU_REL64
            relative64                      R_AMDGPU_RELATIVE64
        x86_64_*
            16                              R_X86_64_16
            32                              R_X86_64_32
            32s                             R_X86_64_32S
            64                              R_X86_64_64
            8                               R_X86_64_8
            copy                            R_X86_64_COPY
            dtpmod64                        R_X86_64_DTPMOD64
            dtpoff32                        R_X86_64_DTPOFF32
            dtpoff64                        R_X86_64_DTPOFF64
            glob_dat                        R_X86_64_GLOB_DAT
            gnu_vtentry                     R_X86_64_GNU_VTENTRY
            gnu_vtinherit                   R_X86_64_GNU_VTINHERIT
            got32                           R_X86_64_GOT32
            got64                           R_X86_64_GOT64
            gotoff64                        R_X86_64_GOTOFF64
            gotpc32                         R_X86_64_GOTPC32
            gotpc32_tlsdesc                 R_X86_64_GOTPC32_TLSDESC
            gotpc64                         R_X86_64_GOTPC64
            gotpcrel                        R_X86_64_GOTPCREL
            gotpcrel64                      R_X86_64_GOTPCREL64
            gotplt64                        R_X86_64_GOTPLT64
            gottpoff                        R_X86_64_GOTTPOFF
            irelative                       R_X86_64_IRELATIVE
            jump_slot                       R_X86_64_JUMP_SLOT
            none                            R_X86_64_NONE
            pc16                            R_X86_64_PC16
            pc32                            R_X86_64_PC32
            pc64                            R_X86_64_PC64
            pc8                             R_X86_64_PC8
            plt32                           R_X86_64_PLT32
            pltoff64                        R_X86_64_PLTOFF64
            relative                        R_X86_64_RELATIVE
            tlsdesc                         R_X86_64_TLSDESC
            tlsdesc_call                    R_X86_64_TLSDESC_CALL
            tlsgd                           R_X86_64_TLSGD
            tlsld                           R_X86_64_TLSLD
            tpoff32                         R_X86_64_TPOFF32
            tpoff64                         R_X86_64_TPOFF64
        aarch64_* // AArch64
            abs16                           R_AARCH64_ABS16
            abs32                           R_AARCH64_ABS32
            abs64                           R_AARCH64_ABS64
            add_abs_lo12_nc                 R_AARCH64_ADD_ABS_LO12_NC
            adr_got_page                    R_AARCH64_ADR_GOT_PAGE
            adr_prel_lo21                   R_AARCH64_ADR_PREL_LO21
            adr_prel_pg_hi21                R_AARCH64_ADR_PREL_PG_HI21
            adr_prel_pg_hi21_nc             R_AARCH64_ADR_PREL_PG_HI21_NC
            call26                          R_AARCH64_CALL26
            condbr19                        R_AARCH64_CONDBR19
            copy                            R_AARCH64_COPY
            glob_dat                        R_AARCH64_GLOB_DAT
            got_ld_prel19                   R_AARCH64_GOT_LD_PREL19
            gotrel32                        R_AARCH64_GOTREL32
            gotrel64                        R_AARCH64_GOTREL64
            jump26                          R_AARCH64_JUMP26
            jump_slot                       R_AARCH64_JUMP_SLOT
            ld64_got_lo12_nc                R_AARCH64_LD64_GOT_LO12_NC
            ld64_gotoff_lo15                R_AARCH64_LD64_GOTOFF_LO15
            ld64_gotpage_lo15               R_AARCH64_LD64_GOTPAGE_LO15
            ld_prel_lo19                    R_AARCH64_LD_PREL_LO19
            ldst128_abs_lo12_nc             R_AARCH64_LDST128_ABS_LO12_NC
            ldst16_abs_lo12_nc              R_AARCH64_LDST16_ABS_LO12_NC
            ldst32_abs_lo12_nc              R_AARCH64_LDST32_ABS_LO12_NC
            ldst64_abs_lo12_nc              R_AARCH64_LDST64_ABS_LO12_NC
            ldst8_abs_lo12_nc               R_AARCH64_LDST8_ABS_LO12_NC
            movw_gotoff_g0                  R_AARCH64_MOVW_GOTOFF_G0
            movw_gotoff_g0_nc               R_AARCH64_MOVW_GOTOFF_G0_NC
            movw_gotoff_g1                  R_AARCH64_MOVW_GOTOFF_G1
            movw_gotoff_g1_nc               R_AARCH64_MOVW_GOTOFF_G1_NC
            movw_gotoff_g2                  R_AARCH64_MOVW_GOTOFF_G2
            movw_gotoff_g2_nc               R_AARCH64_MOVW_GOTOFF_G2_NC
            movw_gotoff_g3                  R_AARCH64_MOVW_GOTOFF_G3
            movw_prel_g0                    R_AARCH64_MOVW_PREL_G0
            movw_prel_g0_nc                 R_AARCH64_MOVW_PREL_G0_NC
            movw_prel_g1                    R_AARCH64_MOVW_PREL_G1
            movw_prel_g1_nc                 R_AARCH64_MOVW_PREL_G1_NC
            movw_prel_g2                    R_AARCH64_MOVW_PREL_G2
            movw_prel_g2_nc                 R_AARCH64_MOVW_PREL_G2_NC
            movw_prel_g3                    R_AARCH64_MOVW_PREL_G3
            movw_sabs_g0                    R_AARCH64_MOVW_SABS_G0
            movw_sabs_g1                    R_AARCH64_MOVW_SABS_G1
            movw_sabs_g2                    R_AARCH64_MOVW_SABS_G2
            movw_uabs_g0                    R_AARCH64_MOVW_UABS_G0
            movw_uabs_g0_nc                 R_AARCH64_MOVW_UABS_G0_NC
            movw_uabs_g1                    R_AARCH64_MOVW_UABS_G1
            movw_uabs_g1_nc                 R_AARCH64_MOVW_UABS_G1_NC
            movw_uabs_g2                    R_AARCH64_MOVW_UABS_G2
            movw_uabs_g2_nc                 R_AARCH64_MOVW_UABS_G2_NC
            movw_uabs_g3                    R_AARCH64_MOVW_UABS_G3
            none                            R_AARCH64_NONE
            p32_abs32                       R_AARCH64_P32_ABS32
            p32_copy                        R_AARCH64_P32_COPY
            p32_glob_dat                    R_AARCH64_P32_GLOB_DAT
            p32_irelative                   R_AARCH64_P32_IRELATIVE
            p32_jump_slot                   R_AARCH64_P32_JUMP_SLOT
            p32_relative                    R_AARCH64_P32_RELATIVE
            p32_tls_dtpmod                  R_AARCH64_P32_TLS_DTPMOD
            p32_tls_dtprel                  R_AARCH64_P32_TLS_DTPREL
            p32_tls_tprel                   R_AARCH64_P32_TLS_TPREL
            p32_tlsdesc                     R_AARCH64_P32_TLSDESC
            prel16                          R_AARCH64_PREL16
            prel32                          R_AARCH64_PREL32
            prel64                          R_AARCH64_PREL64
            relative                        R_AARCH64_RELATIVE
            tls_dtpmod                      R_AARCH64_TLS_DTPMOD
            tls_dtpmod64                    R_AARCH64_TLS_DTPMOD64
            tls_dtprel                      R_AARCH64_TLS_DTPREL
            tls_dtprel64                    R_AARCH64_TLS_DTPREL64
            tls_tprel                       R_AARCH64_TLS_TPREL
            tls_tprel64                     R_AARCH64_TLS_TPREL64
            tlsdesc                         R_AARCH64_TLSDESC
            tlsdesc_add                     R_AARCH64_TLSDESC_ADD
            tlsdesc_add_lo12                R_AARCH64_TLSDESC_ADD_LO12
            tlsdesc_adr_page21              R_AARCH64_TLSDESC_ADR_PAGE21
            tlsdesc_adr_prel21              R_AARCH64_TLSDESC_ADR_PREL21
            tlsdesc_call                    R_AARCH64_TLSDESC_CALL
            tlsdesc_ld64_lo12               R_AARCH64_TLSDESC_LD64_LO12
            tlsdesc_ld_prel19               R_AARCH64_TLSDESC_LD_PREL19
            tlsdesc_ldr                     R_AARCH64_TLSDESC_LDR
            tlsdesc_off_g0_nc               R_AARCH64_TLSDESC_OFF_G0_NC
            tlsdesc_off_g1                  R_AARCH64_TLSDESC_OFF_G1
            tlsgd_add_lo12_nc               R_AARCH64_TLSGD_ADD_LO12_NC
            tlsgd_adr_page21                R_AARCH64_TLSGD_ADR_PAGE21
            tlsgd_adr_prel21                R_AARCH64_TLSGD_ADR_PREL21
            tlsgd_movw_g0_nc                R_AARCH64_TLSGD_MOVW_G0_NC
            tlsgd_movw_g1                   R_AARCH64_TLSGD_MOVW_G1
            tlsie_adr_gottprel_page21       R_AARCH64_TLSIE_ADR_GOTTPREL_PAGE21
            tlsie_ld64_gottprel_lo12_nc     R_AARCH64_TLSIE_LD64_GOTTPREL_LO12_NC
            tlsie_ld_gottprel_prel19        R_AARCH64_TLSIE_LD_GOTTPREL_PREL19
            tlsie_movw_gottprel_g0_nc       R_AARCH64_TLSIE_MOVW_GOTTPREL_G0_NC
            tlsie_movw_gottprel_g1          R_AARCH64_TLSIE_MOVW_GOTTPREL_G1
            tlsld_add_dtprel_hi12           R_AARCH64_TLSLD_ADD_DTPREL_HI12
            tlsld_add_dtprel_lo12           R_AARCH64_TLSLD_ADD_DTPREL_LO12
            tlsld_add_dtprel_lo12_nc        R_AARCH64_TLSLD_ADD_DTPREL_LO12_NC
            tlsld_add_lo12_nc               R_AARCH64_TLSLD_ADD_LO12_NC
            tlsld_adr_page21                R_AARCH64_TLSLD_ADR_PAGE21
            tlsld_adr_prel21                R_AARCH64_TLSLD_ADR_PREL21
            tlsld_ld_prel19                 R_AARCH64_TLSLD_LD_PREL19
            tlsld_ldst128_dtprel_lo12       R_AARCH64_TLSLD_LDST128_DTPREL_LO12
            tlsld_ldst128_dtprel_lo12_nc    R_AARCH64_TLSLD_LDST128_DTPREL_LO12_NC
            tlsld_ldst16_dtprel_lo12        R_AARCH64_TLSLD_LDST16_DTPREL_LO12
            tlsld_ldst16_dtprel_lo12_nc     R_AARCH64_TLSLD_LDST16_DTPREL_LO12_NC
            tlsld_ldst32_dtprel_lo12        R_AARCH64_TLSLD_LDST32_DTPREL_LO12
            tlsld_ldst32_dtprel_lo12_nc     R_AARCH64_TLSLD_LDST32_DTPREL_LO12_NC
            tlsld_ldst64_dtprel_lo12        R_AARCH64_TLSLD_LDST64_DTPREL_LO12
            tlsld_ldst64_dtprel_lo12_nc     R_AARCH64_TLSLD_LDST64_DTPREL_LO12_NC
            tlsld_ldst8_dtprel_lo12         R_AARCH64_TLSLD_LDST8_DTPREL_LO12
            tlsld_ldst8_dtprel_lo12_nc      R_AARCH64_TLSLD_LDST8_DTPREL_LO12_NC
            tlsld_movw_dtprel_g0            R_AARCH64_TLSLD_MOVW_DTPREL_G0
            tlsld_movw_dtprel_g0_nc         R_AARCH64_TLSLD_MOVW_DTPREL_G0_NC
            tlsld_movw_dtprel_g1            R_AARCH64_TLSLD_MOVW_DTPREL_G1
            tlsld_movw_dtprel_g1_nc         R_AARCH64_TLSLD_MOVW_DTPREL_G1_NC
            tlsld_movw_dtprel_g2            R_AARCH64_TLSLD_MOVW_DTPREL_G2
            tlsld_movw_g0_nc                R_AARCH64_TLSLD_MOVW_G0_NC
            tlsld_movw_g1                   R_AARCH64_TLSLD_MOVW_G1
            tlsle_add_tprel_hi12            R_AARCH64_TLSLE_ADD_TPREL_HI12
            tlsle_add_tprel_lo12            R_AARCH64_TLSLE_ADD_TPREL_LO12
            tlsle_add_tprel_lo12_nc         R_AARCH64_TLSLE_ADD_TPREL_LO12_NC
            tlsle_ldst128_tprel_lo12        R_AARCH64_TLSLE_LDST128_TPREL_LO12
            tlsle_ldst128_tprel_lo12_nc     R_AARCH64_TLSLE_LDST128_TPREL_LO12_NC
            tlsle_ldst16_tprel_lo12         R_AARCH64_TLSLE_LDST16_TPREL_LO12
            tlsle_ldst16_tprel_lo12_nc      R_AARCH64_TLSLE_LDST16_TPREL_LO12_NC
            tlsle_ldst32_tprel_lo12         R_AARCH64_TLSLE_LDST32_TPREL_LO12
            tlsle_ldst32_tprel_lo12_nc      R_AARCH64_TLSLE_LDST32_TPREL_LO12_NC
            tlsle_ldst64_tprel_lo12         R_AARCH64_TLSLE_LDST64_TPREL_LO12
            tlsle_ldst64_tprel_lo12_nc      R_AARCH64_TLSLE_LDST64_TPREL_LO12_NC
            tlsle_ldst8_tprel_lo12          R_AARCH64_TLSLE_LDST8_TPREL_LO12
            tlsle_ldst8_tprel_lo12_nc       R_AARCH64_TLSLE_LDST8_TPREL_LO12_NC
            tlsle_movw_tprel_g0             R_AARCH64_TLSLE_MOVW_TPREL_G0
            tlsle_movw_tprel_g0_nc          R_AARCH64_TLSLE_MOVW_TPREL_G0_NC
            tlsle_movw_tprel_g1             R_AARCH64_TLSLE_MOVW_TPREL_G1
            tlsle_movw_tprel_g1_nc          R_AARCH64_TLSLE_MOVW_TPREL_G1_NC
            tlsle_movw_tprel_g2             R_AARCH64_TLSLE_MOVW_TPREL_G2
            tstbr14                         R_AARCH64_TSTBR14

// Legal values for d_tag (dynamic entry type).
AT_NULL          = 0;  // End of vector
AT_IGNORE        = 1;  // Entry should be ignored
AT_EXECFD        = 2;  // File descriptor of program
AT_PHDR          = 3;  // Program headers for program
AT_PHENT         = 4;  // Size of program header entry
AT_PHNUM         = 5;  // Number of program headers
AT_PAGESZ        = 6;  // System page size
AT_BASE          = 7;  // Base address of interpreter
AT_FLAGS         = 8;  // Flags
AT_ENTRY         = 9;  // Entry point of program
AT_NOTELF        = 10; // Program is not ELF
AT_UID           = 11; // Real uid
AT_EUID          = 12; // Effective uid
AT_GID           = 13; // Real gid
AT_EGID          = 14; // Effective gid
AT_CLKTCK        = 17; // Frequency of times()
AT_PLATFORM      = 15; // String identifying platform.
AT_HWCAP         = 16; // Hints about processor capabilities.
AT_FPUCW         = 18; // Used FPU control word.
AT_DCACHEBSIZE   = 19; // Data cache block size.
AT_ICACHEBSIZE   = 20; // Instruction cache block size.
AT_UCACHEBSIZE   = 21; // Unified cache block size.
AT_IGNOREPPC     = 22; // Entry should be ignored.
AT_SECURE        = 23; // Boolean, was exec setuid-like?
AT_BASE_PLATFORM = 24; // String identifying real platforms.
AT_RANDOM        = 25; // Address of 16 random bytes.
AT_HWCAP2  = 26; // More hints about processor capabilities.
AT_EXECFN  = 31; // Filename of executable.
AT_SYSINFO = 32; // EP to the system call in the vDSO.
AT_SYSINFO_EHDR = 33; // Start of the ELF header of the vDSO.
AT_L1I_CACHESHAPE    = 34;
AT_L1D_CACHESHAPE    = 35;
AT_L2_CACHESHAPE     = 36;
AT_L3_CACHESHAPE     = 37;
AT_L1I_CACHESIZE     = 40;
AT_L1I_CACHEGEOMETRY = 41;
AT_L1D_CACHESIZE     = 42;
AT_L1D_CACHEGEOMETRY = 43;
AT_L2_CACHESIZE      = 44;
AT_L2_CACHEGEOMETRY  = 45;
AT_L3_CACHESIZE      = 46;


--enable-feature    [ibt|shstk|lam_u48|lam_u57]
--disable-feature   [ibt|shstk|lam_u48|lam_u57]




*/


template <std::ranges::range R,
          class T = typename std::ranges::range_value_t<R>::value_type>
auto match_arg(const R& options, std::string_view sv)
{
    argument<T> m;
    for (auto&& opt : options) {
        if ((m = opt.match(sv)))
            break;
    }
    return m;
}


struct action
{
    virtual void execute(elfio&) = 0;
};

struct action_option
{
    using action_type = std::shared_ptr<action>;
    using parser_type = std::function<action_type(std::string_view)>;
    using registry_type = std::map<std::string_view, parser_type>;
    static registry_type registry;

    std::string_view option;

    action_option(std::string_view option, parser_type parser)
        : option { option }
    {
        registry.emplace(option, parser);
    }
};

action_option::registry_type action_option::registry {};



Elf_Word get_section_index(elfio& elf, std::string_view name)
{
    if (name == "-")
        return SHN_UNDEF;
    auto&& section = *(elf.sections | std::views::filter([&name](auto&& s) {
        return s->get_name() == name;
    })).begin();
    if (section == nullptr)
        throw std::out_of_range { "could not find section" };
    return section->get_index();
}

auto get_symbol_section(elfio& elf)
{
    auto&& section = *(elf.sections | std::views::filter([](auto&& s) {
        return s->get_type() == SHT_SYMTAB;
    })).begin();
    if (section == nullptr)
        throw std::out_of_range { "could not find symbol table" };
    return section.get();
}






struct add_symbol_base
    : public action
{
    std::string     name;
    Elf64_Addr      value;
    Elf_Xword       size;
    unsigned char   other;
    std::string     symsec;

    add_symbol_base(std::string_view name, Elf64_Addr value, Elf_Xword size,
                    unsigned char other, std::string_view symsec)
        : name { name }
        , value { value }
        , size { size }
        , other { other }
        , symsec { symsec }
    {}
};

struct add_symbol_info
    : public add_symbol_base
{
    using ptr_type = std::shared_ptr<add_symbol_info>;
    static constexpr auto parser = make_instance_parser<ptr_type>(
        lookup::elf::symbol::name,
        lookup::elf::symbol::value.with_default(0),
        lookup::elf::symbol::size.with_default(0),
        lookup::elf::symbol::info,
        lookup::elf::symbol::visibility.with_default(STV_DEFAULT),
        lookup::elf::symbol::section.with_default("-")
    );

    unsigned char info;

    add_symbol_info(std::string_view name, Elf64_Addr value, Elf_Xword size,
                    unsigned char info, unsigned char other,
                    std::string_view symsec)
        : add_symbol_base { name, value, size, other, symsec }
        , info { info }
    {}

    virtual void execute(elfio& elf)
    {
        auto symbols = get_symbol_section(elf);
        auto strings = elf.sections[symbols->get_link()];
        auto shndx = get_section_index(elf, symsec);

        better_string_section_accessor strtab { strings };
        better_symbol_section_accessor symtab { elf, symbols };

        symtab.add_symbol(strtab, name, value, size, info, other, shndx);
    }
};

struct add_symbol_bind_type
    : public add_symbol_base
{
    using ptr_type = std::shared_ptr<add_symbol_bind_type>;
    static constexpr auto parser = make_instance_parser<ptr_type>(
        lookup::elf::symbol::name,
        lookup::elf::symbol::value.with_default(0),
        lookup::elf::symbol::size.with_default(0),
        lookup::elf::symbol::binding.with_default(STB_GLOBAL),
        lookup::elf::symbol::type.with_default(STT_NOTYPE),
        lookup::elf::symbol::visibility.with_default(STV_DEFAULT),
        lookup::elf::symbol::section.with_default("-")
    );

    unsigned char bind;
    Elf_Half type;

    add_symbol_bind_type(std::string_view name, Elf64_Addr value,
                         Elf_Xword size, unsigned char bind, Elf_Half type,
                         unsigned char other, std::string_view symsec)
        : add_symbol_base { name, value, size, other, symsec }
        , bind { bind }
        , type { type }
    {}

    virtual void execute(elfio& elf)
    {
        auto symbols = get_symbol_section(elf);
        auto strings = elf.sections[symbols->get_link()];
        auto shndx = get_section_index(elf, symsec);

        better_string_section_accessor strtab { strings };
        better_symbol_section_accessor symtab { elf, symbols };

        symtab.add_symbol(strtab, name, value, size, bind, type, other, shndx);
    }
};


struct add_symbol
    : public action
{
    using ptr_type = std::shared_ptr<add_symbol_base>;
    static constexpr auto parser = make_choice_parser<ptr_type>(
        add_symbol_info::parser,
        add_symbol_bind_type::parser
    );

    static action_option entry;
    static std::shared_ptr<action> parse(std::string_view);
};

action_option add_symbol::entry { "add-symbol", add_symbol::parse };

std::shared_ptr<action> add_symbol::parse(std::string_view input)
{
    return parser.match(input);
}


struct set_type
    : public action
{
    static action_option entry;
    static std::shared_ptr<action> parse(std::string_view);

    Elf_Half type;

    set_type(Elf_Half type)
        : type { type }
    {}

    virtual void execute(elfio& elf)
    {
        elf.set_type(type);
    }
};

action_option set_type::entry { "set-type", set_type::parse };

std::shared_ptr<action> set_type::parse(std::string_view input)
{
    return std::make_shared<set_type>(lookup::elf::type.match(input));
}


struct set_osabi
    : public action
{
    static action_option entry;
    static std::shared_ptr<action> parse(std::string_view);

    unsigned char osabi;

    set_osabi(unsigned char osabi)
        : osabi { osabi }
    {}

    virtual void execute(elfio& elf)
    {
        elf.set_os_abi(osabi);
    }
};

action_option set_osabi::entry { "set-osabi", set_osabi::parse };

std::shared_ptr<action> set_osabi::parse(std::string_view input)
{
    auto&& match = match_arg(lookup::elf::osabi, input);
    if (match) {
        return std::make_shared<set_osabi>(match.value());
    } else {
        throw std::invalid_argument { "invalid OSABI" };
    }
}


struct set_abiversion
    : public action
{
    static action_option entry;
    static std::shared_ptr<action> parse(std::string_view);

    unsigned char abiversion;

    set_abiversion(unsigned char abiversion)
        : abiversion { abiversion }
    {}

    virtual void execute(elfio& elf)
    {
        elf.set_abi_version(abiversion);
    }
};

action_option set_abiversion::entry { "set-abiversion", set_abiversion::parse };

std::shared_ptr<action> set_abiversion::parse(std::string_view input)
{
    auto&& match = match_arg(lookup::elf::abiversion, input);
    if (match) {
        return std::make_shared<set_abiversion>(match.value());
    } else {
        throw std::invalid_argument { "invalid ABI version" };
    }
}


struct set_machine
    : public action
{
    static action_option entry;
    static std::shared_ptr<action> parse(std::string_view);

    Elf_Half machine;

    set_machine(Elf_Half machine)
        : machine { machine }
    {}

    virtual void execute(elfio& elf)
    {
        elf.set_machine(machine);
    }
};

action_option set_machine::entry { "set-machine", set_machine::parse };

std::shared_ptr<action> set_machine::parse(std::string_view input)
{
    auto&& match = match_arg(lookup::elf::machine, input);
    if (match) {
        return std::make_shared<set_machine>(match.value());
    } else {
        throw std::invalid_argument { "invalid machine" };
    }
}


namespace lookup {
constexpr auto branding = value_parser<std::string, std::string_view> { false };
};


#if 0
struct set_branding
    : public action
{
    static action_option entry;
    static std::shared_ptr<action> parse(std::string_view);

    std::string branding;

    set_branding(std::string_view branding)
        : branding { branding }
    {}

    virtual void execute(elfio& elf)
    {
        // XXX: this is an ancient feature, no accessor normally exists
        memset(&elf.get_ident()[EI_ABIVERSION], 0, EI_NIDENT-EI_ABIVERSION);
        memcpy(&elf.get_ident()[EI_ABIVERSION], branding.data(),
               std::min(branding.size(), (size_t)(EI_NIDENT-EI_ABIVERSION)));
    }
};

action_option set_branding::entry { "set-branding", set_branding::parse };

std::shared_ptr<action> set_branding::parse(std::string_view input)
{
    return std::make_shared<set_branding>(lookup::branding.match(input));
}
#endif


