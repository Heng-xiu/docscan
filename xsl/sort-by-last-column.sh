#!/usr/bin/env bash

# Copyright (2017) Thomas Fischer <thomas.fischer@his.se>, senior
# lecturer at University of Skövde, as part of the LIM-IT project.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

# This script does the following:
# 1. Read a tab-separated .csv file from stdin and optionally take a 'number of
#    header lines' as command line argument (if omitted, 2 is the default).
# 2. For all lines following the header lines, separate columns by tab character
#    and sort rows numerically by last column's value.
# 3. Dump sorted rows.

export tempfile=$(mktemp --tmpdir 'sort-by-last-column-XXXXXXXXXXXXXXXX.csv')
function cleanup_on_exit {
	rm -f ${tempfile}
}
trap cleanup_on_exit EXIT

# two header lines by default, unless a single argument is passed with a different value
numheaderlines=${1:-2}

# read from stdin, redirect to temporary file
while read line ; do echo "${line}" ; done >${tempfile}

head -n ${numheaderlines} ${tempfile}
tail -n +$((${numheaderlines} +1)) ${tempfile} | awk -F '[\t]' '{printf $NF ; for (c=1;c<NF;++c) printf "\t"$c ; print ""}' | sort -nr | awk -F '[\t]' '{for (c=2;c<=NF;++c) printf $c"\t" ; print $1}'
