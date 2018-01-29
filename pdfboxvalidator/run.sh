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

export version=${version:-2.0.8}

cd "$(dirname "${0}")"
./build.sh

jarfiles=$(ls -1 *.jar | xargs printf "%s:" ; echo ".")

xmloption=

for pdffile in "${@}" ; do
	if [ "${pdffile}" = "--xml" ] ; then
		xmloption="--xml"
		continue
	fi

	test "${pdffile}" = "${pdffile/.pdf}" && continue
	java -cp "${jarfiles}" PdfBoxValidator "${xmloption}" "${pdffile}"
	exitcode=$?
	test ${exitcode} -eq 0 || exit ${exitcode}
done
