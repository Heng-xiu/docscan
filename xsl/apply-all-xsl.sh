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
# 3. For each resulting .xml file, apply all .xsl files in the script's directory
# 4. Resulting .csv files will be located next to the original .xml/.xml.xz file,
#    with the filename enhanced with the xsl filename that was used to generate
#    the .csv file
# This script is parallelized as much as possible using the 'queue' command.
# See here: https://gitlab.com/tfscripts/linuxcommandline

export tempdir=$(mktemp --tmpdir -d 'apply-all-xsl-XXXXXXXXXXXXXXXX.d')
function cleanup_on_exit {
	rm -rf ${tempdir}
}
trap cleanup_on_exit EXIT

# Limit memory usage to ${memlimitGB} GB
memlimitGB=6
memlimitB=$(( ${memlimitGB} * 1024 * 1024 * 1024 ))
memlimitPages=$(( ${memlimitB} / $(getconf PAGE_SIZE) ))
renice -n 10 $$
ionice -c 3 -p $$

for xmlfile in "${@}" ; do
	stem="${xmlfile/.xml.xz/}"
	stem="${stem/.xml/}"

	echo -n "." >&2
	thisxml=${tempdir}/$(md5sum <<<"${xmlfile}" | cut -f 1 -d ' ')".xml"

	if [[ "${xmlfile}" =~ '.xml.xz' ]] ; then echo -n "nice -n 15 ionice -c 3 unxz <\"${xmlfile}\" >${thisxml} || exit 1"
	elif [[ "${xmlfile}" =~ '.xml' ]] ; then echo -n "cp \"${xmlfile}\" ${thisxml} || exit 1"
	else echo -n "rm -f ${thisxml} ; exit 1" ; fi >>${tempdir}/q1.txt
	echo -n " ; test -s ${thisxml} || exit 1" >>${tempdir}/q1.txt

	echo " ; nice sed -i -r 's![\d1\d6\d3\d11\d12\d14\d15\d16\d17\d19\d20\d21\d22\d23\d25\d27\d28\d29]!_!g;s!<error([ ][^>]*)?>[^<]*</error>!!g' ${thisxml} || exit 1" >>${tempdir}/q1.txt

	for xslfile in "$(dirname "$0")"/*.xsl ; do
		extension="csv"
		[[ "${xslfile}" =~ '-to-html.xsl' ]] && extension="html"
		outputfile="${stem}-$(basename "${xslfile/.xsl/}").${extension}"
		echo "( echo \$\$ | sudo -n /usr/bin/tee /sys/fs/cgroup/memory/xsltprocsandbox/cgroup.procs | sudo -n /usr/bin/tee /sys/fs/cgroup/cpu/xsltprocsandbox/cgroup.procs >/dev/null || exit 1 ; prlimit --rss="${memlimitPages}" xsltproc \"${xslfile}\" \"${thisxml}\" >\"${outputfile}\" || { rm -f \"${outputfile}\" ; exit 1 ; } ; )"
	done >>${tempdir}/q2.txt
done
echo >&2

test -d /sys/fs/cgroup/memory/xsltprocsandbox || sudo mkdir /sys/fs/cgroup/memory/xsltprocsandbox || exit 1
test -f /sys/fs/cgroup/memory/xsltprocsandbox/memory.limit_in_bytes && test -f /sys/fs/cgroup/memory/xsltprocsandbox/memory.memsw.limit_in_bytes && { echo ${memlimitB} | sudo tee /sys/fs/cgroup/memory/xsltprocsandbox/memory.memsw.limit_in_bytes >/dev/null ; }
echo ${memlimitB} | sudo tee /sys/fs/cgroup/memory/xsltprocsandbox/memory.limit_in_bytes >/dev/null || exit 1
echo ${memlimitB} | sudo tee /sys/fs/cgroup/memory/xsltprocsandbox/memory.memsw.limit_in_bytes >/dev/null || exit 1

# Limit to 90% CPU time ('xsltproc' is a single core process)
test -d /sys/fs/cgroup/cpu/xsltprocsandbox || sudo mkdir /sys/fs/cgroup/cpu/xsltprocsandbox || exit 1
echo 900 | sudo tee /sys/fs/cgroup/cpu/xsltprocsandbox/cpu.shares >/dev/null || exit 1

randline -q <${tempdir}/q1.txt >${tempdir}/q-rand.txt
echo "WAIT" >>${tempdir}/q-rand.txt
randline -q <${tempdir}/q2.txt >>${tempdir}/q-rand.txt
queue -V -j 1 -s ${tempdir}/q-rand.txt
