#!/usr/bin/env bash

## Copyright (2017) Thomas Fischer <thomas.fischer@his.se>, senior
## lecturer at University of Skövde, as part of the LIM-IT project.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## 1. Redistributions of source code must retain the above copyright notice, this
##    list of conditions and the following disclaimer.
## 2. Redistributions in binary form must reproduce the above copyright notice,
##    this list of conditions and the following disclaimer in the documentation
##    and/or other materials provided with the distribution.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
## ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
## WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
## DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
## ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
## (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
## LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
## ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
## (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
## SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

export version=${version:-2.0.7}

cd "$(dirname "${0}")"

for stem in fontbox pdfbox preflight xmpbox ; do
	fn="${stem}-${version}.jar"
	test -s "${fn}" && continue
	wget "https://archive.apache.org/dist/pdfbox/${version}/${fn}" || exit 1
done
for tool in commons-logging-1.2 commons-io-2.5 ; do
	fn="${tool}.jar"
	test -s "${fn}" && continue
	stem=$(echo "${tool}" | sed -e 's!^commons-!!g;s!-[1-9][0-9]*[.][0-9]*$!!g')
	rm -rf /tmp/pdfbox-dl ; mkdir -p /tmp/pdfbox-dl
	( cd /tmp/pdfbox-dl && wget http://apache.mirrors.spacedump.net/commons/${stem}/binaries/${tool}-bin.zip ) || exit 1
	( cd /tmp/pdfbox-dl && unzip ${tool}-bin.zip ) || exit 1
	find /tmp/pdfbox-dl -name "${fn}" -exec cp '{}' . ';' || exit 1
	rm -rf /tmp/pdfbox-dl
done

jarfiles=$(ls -1 *.jar | xargs printf "%s:" ; echo ".")

if [[ ! -s PdfBoxValidator.class || PdfBoxValidator.java -nt PdfBoxValidator.class ]] ; then javac -cp "${jarfiles}" PdfBoxValidator.java ; fi
exit $?
