#! /bin/sh
# Copyright (C) 2010, 2011 Red Hat, Inc.
# This file is part of Red Hat elfutils.
#
# Red Hat elfutils is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# Red Hat elfutils is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with Red Hat elfutils; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301 USA.
#
# Red Hat elfutils is an included package of the Open Invention Network.
# An included package of the Open Invention Network is a package for which
# Open Invention Network licensees cross-license their patents.  No patent
# license is granted, either expressly or impliedly, by designation as an
# included package.  Should you wish to participate in the Open Invention
# Network licensing program, please visit www.openinventionnetwork.com
# <http://www.openinventionnetwork.com>.

. $srcdir/../tests/test-subr.sh

srcdir=$srcdir/tests

testfiles libdl-2.12.so.debug

# Here we test that dwarflint can tolerate invalid attribute name.
testrun_compare ./dwarflint --check=@low --nognu --ignore-bloat libdl-2.12.so.debug <<EOF
error: .debug_abbrev: abbr. attribute 0xbe: invalid or unknown name 0x2107.
error: .debug_abbrev: abbr. attribute 0x330: invalid or unknown name 0x2107.
error: .debug_abbrev: abbr. attribute 0xa28: invalid or unknown name 0x2107.
error: .debug_abbrev: abbr. attribute 0x108e: invalid or unknown name 0x2107.
error: .debug_abbrev: abbr. attribute 0x1300: invalid or unknown name 0x2107.
warning: .debug_info: CU 55709: no aranges table is associated with this CU.
warning: .debug_info: CU 56524: no aranges table is associated with this CU.
EOF

# Here we test proper support for DW_AT_GNU_vector
testrun_compare ./dwarflint --check=@low --ignore-bloat libdl-2.12.so.debug <<EOF
warning: .debug_info: CU 55709: no aranges table is associated with this CU.
warning: .debug_info: CU 56524: no aranges table is associated with this CU.
EOF
