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

#include <cstdint>
#include <utility>
#include <concepts>
#include <ostream>

enum class endian
{
    big,
    little,
    mixed,
    native
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    = little,
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    = big,
#else
#error "Unknown host endianness, cannot compile."
#endif
};

namespace detail {

union helper16
{
    uint16_t u;
    int16_t  i;
    uint8_t  b[2];
};
union helper32
{
    uint32_t u;
    int32_t  i;
    uint8_t  b[4];
    helper16 q[2];
};
union helper64
{
    uint64_t u;
    int64_t  i;
    uint8_t  b[8];
    helper16 q[4];
};

} // ::detail

template <size_t I, endian From, endian To>
struct endian_swap;

template <size_t I, endian Endian>
    requires (I > 1)
struct endian_swap<I, Endian, Endian>
{
    template <class T>
    static constexpr T swap(T value)
    { return value; }
};

template <size_t I>
    requires (I > 1)
struct endian_swap<I, endian::little, endian::big>
{
    using base = endian_swap<I, endian::big, endian::little>;
    template <class T>
    static constexpr T swap(T value)
    { return base::swap(value); }
};

template <size_t I, endian To>
    requires (I > 1)
struct endian_swap<I, endian::mixed, To>
{
    using base = endian_swap<I, To, endian::mixed>;
    template <class T>
    static constexpr T swap(T value)
    { return base::swap(value); }
};

template <endian From, endian To>
struct endian_swap<1, From, To>
{
    template <class T>
    static constexpr T swap(T value)
    { return value; }
};


template <>
struct endian_swap<2, endian::big, endian::little>
{
    static constexpr auto _swap(detail::helper16&& h)
    {
        std::swap(h.b[0], h.b[1]);
        return std::forward<detail::helper16>(h);
    }
    template <std::unsigned_integral U>
    static constexpr uint16_t swap(U value)
    { return _swap({ .u = value }).u; }
    template <std::signed_integral S>
    static constexpr int16_t swap(S value)
    { return _swap({ .i = value }).i; }
};

template <>
struct endian_swap<4, endian::big, endian::little>
{
    static constexpr auto _swap(detail::helper32&& h)
    {
        std::swap(h.b[0], h.b[3]);
        std::swap(h.b[1], h.b[2]);
        return std::forward<detail::helper32>(h);
    }
    template <std::unsigned_integral U>
    static constexpr uint32_t swap(U value)
    { return _swap({ .u = value }).u; }
    template <std::signed_integral S>
    static constexpr int32_t swap(S value)
    { return _swap({ .i = value }).i; }
};

template <>
struct endian_swap<8, endian::big, endian::little>
{
    static constexpr auto _swap(detail::helper64&& h)
    {
        std::swap(h.b[0], h.b[7]);
        std::swap(h.b[1], h.b[6]);
        std::swap(h.b[2], h.b[5]);
        std::swap(h.b[3], h.b[4]);
        return std::forward<detail::helper64>(h);
    }
    template <std::unsigned_integral U>
    static constexpr uint64_t swap(U value)
    { return _swap({ .u = value }).u; }
    template <std::signed_integral S>
    static constexpr int64_t swap(S value)
    { return _swap({ .i = value }).i; }
};

template <>
struct endian_swap<2, endian::big, endian::mixed>
{
    static constexpr auto _swap(detail::helper16&& h)
    {
        std::swap(h.b[0], h.b[1]);
        return std::forward<detail::helper16>(h);
    }
    template <std::unsigned_integral U>
    static constexpr uint16_t swap(U value)
    { return _swap({ .u = value }).u; }
    template <std::signed_integral S>
    static constexpr int16_t swap(S value)
    { return _swap({ .i = value }).i; }
};

template <>
struct endian_swap<4, endian::big, endian::mixed>
{
    static constexpr auto _swap(detail::helper32&& h)
    {
        std::swap(h.b[0], h.b[1]);
        std::swap(h.b[2], h.b[3]);
        return std::forward<detail::helper32>(h);
    }
    template <std::unsigned_integral U>
    static constexpr uint32_t swap(U value)
    { return _swap({ .u = value }).u; }
    template <std::signed_integral S>
    static constexpr int32_t swap(S value)
    { return _swap({ .i = value }).i; }
};

template <>
struct endian_swap<8, endian::big, endian::mixed>
{
    static constexpr auto _swap(detail::helper64&& h)
    {
        std::swap(h.b[0], h.b[1]);
        std::swap(h.b[2], h.b[3]);
        std::swap(h.b[4], h.b[5]);
        std::swap(h.b[6], h.b[7]);
        return std::forward<detail::helper64>(h);
    }
    template <std::unsigned_integral U>
    static constexpr uint64_t swap(U value)
    { return _swap({ .u = value }).u; }
    template <std::signed_integral S>
    static constexpr int64_t swap(S value)
    { return _swap({ .i = value }).i; }
};

template <>
struct endian_swap<2, endian::little, endian::mixed>
{
    template <std::unsigned_integral U>
    static constexpr uint16_t swap(U value)
    { return value; }
    template <std::signed_integral S>
    static constexpr int16_t swap(S value)
    { return value; }
};

template <>
struct endian_swap<4, endian::little, endian::mixed>
{
    static constexpr auto _swap(detail::helper32&& h)
    {
        std::swap(h.q[0].u, h.q[1].u);
        return std::forward<detail::helper32>(h);
    }
    template <std::unsigned_integral U>
    static constexpr uint32_t swap(U value)
    { return _swap({ .u = value }).u; }
    template <std::signed_integral S>
    static constexpr int32_t swap(S value)
    { return _swap({ .i = value }).i; }
};

template <>
struct endian_swap<8, endian::little, endian::mixed>
{
    static constexpr auto _swap(detail::helper64&& h)
    {
        std::swap(h.q[0].u, h.q[3].u);
        std::swap(h.q[1].u, h.q[2].u);
        return std::forward<detail::helper64>(h);
    }
    template <std::unsigned_integral U>
    static constexpr uint64_t swap(U value)
    { return _swap({ .u = value }).u; }
    template <std::signed_integral S>
    static constexpr int64_t swap(S value)
    { return _swap({ .i = value }).i; }
};

template <endian From, endian To = endian::native, std::integral I>
constexpr auto swap_endian(I value)
{
    return endian_swap<sizeof(I), From, To>::swap(value);
}

template <endian From, endian To = endian::native, std::integral O, std::integral I>
constexpr auto swap_endian_to(I value, const O&)
{
    return endian_swap<sizeof(O), From, To>::swap(static_cast<O>(value));
}

std::ostream& operator<<(std::ostream& os, endian endianness)
{
    switch (endianness)
    {
    case endian::little:
        return os << "little endian";
    case endian::big:
        return os << "big endian";
    case endian::mixed:
        return os << "mixed (PDP) endian";
    default:
        return os << "unknown endian";
    }
}


