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
# 1. Takes a varying number of tab-separated .csv files as command line arguments.
# 2. Prints the first file's first line
# 3. For all files, takes all lines starting from the second one and for each column
#    starting from the second column, take the numeric value, add it in an array
#    (column number is index); the index is stored in a hash where the first column's
#    value is the key.
#    In case the current value is not numeric, just write it as-is into the hash-array
#    data structure.
# 4. As the hash is kept for all files and each first column value is unique within
#    each file, the hash-array data structure aggregates all unique key-column values
#    across all .csv files.
# 5. Finally, print a new tab-separated table that looks like the input tables. This
#    table contains the aggregated values. In case there is a 'total' key (row) in
#    the input files, this row will be printed first.

head -n 1 $1

tail -q -n +2 "${@}" | awk -F '	' '{ for (c=2;c<=NF;++c) { if ($c ~ /^[0-9]+([.][0-9]*)?/) { hash[$1][c]+=$c } else { hash[$1][c]=$c } } } END { if ("total" in hash ) { printf "total" ; for (c=2; c<=10 && (c in hash["total"]); ++c) { printf "	"hash["total"][c] } ; print "" } ; for (key in hash) if (key != "total" ) { printf key ; for (c=2; c<=10 && (c in hash[key]); ++c) { printf "	"hash[key][c] } ; print "" } }'
