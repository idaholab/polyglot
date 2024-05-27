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

#include <string>
#include <concepts>
#include <cstdint>


namespace common {

namespace ancient {

/*
 * Excerpt from BSD ar(5):
 * │ The first [ar format] was denoted by the leading ``magic'' number
 * │ 0177555 (stored as type int). These archives were almost certainly
 * │ created on a 16-bit machine, and contain headers made up of five fields.
 * │ The fields are the object name (8 characters), the file last
 * │ modification time (type long), the user id (type char), the file mode
 * │ (type char) and the file size (type unsigned int). Files were padded to
 * │ an even number of bytes.
 * 
 * FIXME:   I haven't been able to locate any examples of this to play with, so
 *          this implementation is as literal an interpretation of the manpage
 *          struct description as I can manage. If someone has examples of this,
 *          I'd love to actually add support properly... Even going as far back
 *          as the Internet Archive's copy of 1BSD, I've only found the second
 *          format, so this has to be -incredibly- ancient, and may just be
 *          entirely lost to the annals of time.
 */

static constexpr uint16_t magic = 0177555;

struct ar_hdr {
    char     ar_name[8];
    int32_t  ar_date;
    char     ar_uid;
    char     ar_mode;
    uint16_t ar_size;
} __attribute__((packed));

} // ::ancient

namespace old {

/*
 * I've only found 16-bit examples of this from 1BSD/2BSD/3BSD; there is a
 * similar 32-bit format I found for 3BSD, but it's different -enough- to not
 * match that spec, so I've moved it to `bsd::old3` below.
 * 
 * Spec excerpt from BSD ar(5):
 * │ The second was denoted by the leading ``magic'' number 0177545 (stored as
 * │ type int). These archives may have been created on either 16 or 32- bit
 * │ machines, and contain headers made up of six fields.  The fields are the
 * │ object name (14 characters), the file last modification time (type long),
 * │ the user and group id's (each type char), the file mode (type int) and the
 * │ file size (type long). Files were padded to an even number of bytes.
 *
 * FIXME:   Is there actually a 32-bit version of this? Or is it the same
 *          structure, just with 32-bit objects contained within it? I'm
 *          honestly not sure at this point, and don't have examples to base
 *          anything on. If anyone has any, feel free to contribute.
 */

static constexpr uint16_t magic = 0177545;

struct ar_hdr {
    char     ar_name[14];
    int32_t  ar_date;
    char     ar_uid;
    char     ar_gid;
    int16_t  ar_mode;
    int32_t  ar_size;
} __attribute__((packed));

/*
65ff

524541445f4d4500000000000000    name:   READ_ME
d90e 60f8                       mtime:  0x0ed9f860
03                              user:   3
00                              group:  0
a481                            mode:   0100655
0000 4500                       size:   0x45
    54686520726f7574696e657320696e206c6962582e61206172652066726f6d202e2e2f733720
    616e6420617265207573656420627920657820616e6420617368656c6c2e0a00

666c743430000000000000000000    name:   flt40
d90e 5ff8
03
00
ed81
0000 a607
    0701240672015007000000000000010009f080112612d00b36100200f7090a0096250e10df09
    f20501897709de05c6e50802f525030004000703ce150607e615f406df093602d60bf5650200
    0600751f0600f4fff56502000600751f0600f2ffce159607
*/

} // ::old

namespace current {

static constexpr std::string_view magic = "!<arch>\n";
static constexpr std::string_view extended = "#1/";
static constexpr std::string_view fmag = "`\n";

struct ar_hdr {
    char ar_name[16];   /* name */
    char ar_date[12];   /* modification time */
    char ar_uid[6];     /* user id */
    char ar_gid[6];     /* group id */
    char ar_mode[8];    /* octal file permissions */
    char ar_size[10];   /* size in bytes */
    char ar_fmag[2];    /* consistency check */
} __attribute__((packed));

/*

!<arch>

23312f32302020202020202020202020    name:   #1/20
313637373934383732372020            mtime:  1677948727
302020202020                        user:   0
302020202020                        group:  0
3130303634342020                    mode:   100644
36302020202020202020                size:   60
600a                                fmag:   "`\n"
5f5f2e53594d44454620534f5254454400000000    extname:    __.SYMDEF SORTED
000000a0: 1000 0000 0000 0000 8000 0000 0600 0000  ................
000000b0: 9003 0000 1000 0000 5f6d 6169 6e00 5f79  ........_main._y
000000c0: 7965 7272 6f72 0000
2331 2f31 3220 2020  #1/12
000000d0: 2020 2020 2020 2020 3020 2020 2020 2020          0
000000e0: 2020 2020 3020 2020 2020 3020 2020 2020      0     0
000000f0: 3130 3036 3434 2020 3732 3420 2020 2020  100644  724
00000100: 2020 600a 6d61 696e 2e6f 0000 0000 0000    `.main.o......
00000110: cffa edfe 0c00 0001 0000 0000 0100 0000  ................
00000120: 0500 0000 9001 0000 0020 0000 0000 0000  ......... ......
00000130: 1900 0000 3801 0000 0000 0000 0000 0000  ....8...........
00000140: 0000 0000 0000 0000 0000 0000 0000 0000  ................
*/

} // ::current

} // ::common

namespace bsd {

namespace old3 {

/*
I've moved this into its own namespace (old3) because it -might- be the
"mystical missing third format" that the FreeBSD documentation mentions ('There
have been at least four ar formats.') but fails to describe, and it doesn't seem
to match anything else I can find... but I have a bunch of examples from 3BSD.

It's worth noting that libmagic reports this file type as an "HP old archive"
(matching on the leading 65ff0000, but no more), and the related documentation
in the magic DB source would indicate that there may be some question about this
(possibly endianness-related?), but zero indication of where they got that.

Notable differences:
  - the name is a more modern 16 bytes
  - the name is a more modern 16 bytes
  - it's almost assuredly from a 32-bit host architecture, since both `int`
    (mode) and `long` (date/size) are 32-bit fields
  - the (noted) previous limitation of the 8-bit uid/gid fields has been dealt
    with, and they've both been bumped up to 16-bit fields
  - the 
*/

static constexpr uint32_t magic = 0177545;

struct ar_hdr
{
    char     ar_name[16];
    uint32_t ar_date;
    uint16_t ar_uid;
    uint16_t ar_gid;
    uint32_t ar_mode;
    uint32_t ar_size;
};

/*
65ff 0000

6162 6f72 742e 6f00 0000 0000 0000 0000     name: abort.o
96de c912                                   last modification
000a                                        user: 10
0000                                        group: 0
ed81 0000                                   mode: 0100755
3800 0000                                   length: 0x38
    0801000008000000000000000000000010000000000000000000000000000000000000d45004
    00005f61626f7274000005008801000000006162732e6f000000000000000000000098dec912
    000a0000ed810000580000000801000018000000000000000000000020000000000000000000
    0000000000000000d0ac04501803ce505004000070ac04501803725050045f61627300000000
    0500b700000000005f66616273000000050010000c000000

6163 6374 2e6f 0000 0000 0000 0000 0000     acct.o
9bde c912
000a
0000
a481 0000
5800 0000
    08010000100000000000000000000000200000000000000008000000

00000260: 0000 0000 6361 6c6c 6f63 2e6f 0000 0000  ....calloc.o....
00000270: 0000 0000 a5de c912 000a 0000 a481 0000  ................
00000280: b000 0000 0801 0000 4000 0000 0000 0000  ........@.......

636572726f722e6f0000 000000000000  cerror.o
a7de c912
000a
0000
a481 0000
5400 0000
080100000c000000000000000000000020000000000000000800000000000000d050eff9ffffffce01500400030000000100000d636572726f72000005004902000000005f6572726e6f00000100350004000000
*/

} // ::old3

} // ::bsd

