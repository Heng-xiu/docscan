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
# 1. Takes a varying number of .xml or .xml.xz files as command line arguments.
# 2. In case of .xml.xz files, those will be decompressed to .xml files.
# 3. For each processed .xml file:
#    a. Remove XML header and enclosing <log>...</log> tags
#    b. Remove invalid characters (ASCII codes 1 to 31)
#    c. Remove very log error messages
# 4. Dump all 'inner' XML data, but wrap it into a valid XML output, i.e.
#    re-inserting <?xml ...><log>...</log>

export tempxml=$(mktemp --tmpdir 'join-log-xml-XXXXXXXXXXXXXXXX.xml')
function cleanup_on_exit {
	rm -f ${tempxml}
}
trap cleanup_on_exit EXIT

echo '<?xml version="1.0" encoding="UTF-8" ?>'
echo '<log isodate="'$(date -u '+%Y-%m-%dT%H:%M:%SZ')'">'

for xmlfile in "${@}" ; do
	echo "${xmlfile}" >&2
	if [[ "${xmlfile}" =~ '.xml.xz' ]] ; then nice -n 15 ionice -c 3 unxz <"${xmlfile}" >${tempxml} || exit 1
	elif [[ "${xmlfile}" =~ '.xml' ]] ; then cp "${xmlfile}" ${tempxml} || exit 1
	else rm -f ${tempxml} ; fi
	test -s ${tempxml} && { grep -vP '^<([?]xml |/log>|log |!-- 20[12])' ${tempxml} | nice -n 15 sed -r 's![\d1\d6\d3\d11\d12\d14\d15\d16\d17\d19\d20\d21\d22\d23\d25\d27\d28\d29]!_!g' | nice -n 15 sed -r 's!<error([ ][^>]*)?>[^<]*</error>!!g' || exit 1 ; }
done

echo '</log>'
echo '<!-- '$(date -u '+%Y-%m-%dT%H:%M:%SZ')' -->'

