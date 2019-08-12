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

CODEDIR="$(dirname "$0")"

if [[ ! -s ${CODEDIR}/ValidatePDFA_XXXX_.class || ${CODEDIR}/ValidatePDFA_XXXX_.java -nt ${CODEDIR}/ValidatePDFA_XXXX_.class ]] ; then
	echo "Rebuilding Java source code" >&2
	( cd "${CODEDIR}" && javac -Xlint:deprecation -cp commons-text-1.6.jar:jPDFPreflight.jar ValidatePDFA_XXXX_.java ) || exit 1
fi
[ -s "${CODEDIR}/ValidatePDFA_XXXX_.class" ] || { echo "No ValidatePDFA_XXXX_.class found" >&2 ; exit 1 ; }

[ -s "${CODEDIR}/jPDFPreflight.key" ] || { echo "No jPDFPreflight.key found" >&2 ; exit 1 ; }
key="$(cat "${CODEDIR}/jPDFPreflight.key")"
[ -n "${key}" ] || { echo "jPDFPreflight key is empty" >&2 ; exit 1 ; }

if [ $# -eq 0 ] ; then
	java -cp "${CODEDIR}/commons-text-1.6.jar:${CODEDIR}/commons-lang3-3.8.1.jar:${CODEDIR}/jPDFPreflight.jar:${CODEDIR}" ValidatePDFA_XXXX_ "${key}"
	exit 0
fi

for pdffile in "${@}" ; do
	java -cp "${CODEDIR}/commons-text-1.6.jar:${CODEDIR}/commons-lang3-3.8.1.jar:${CODEDIR}/jPDFPreflight.jar:${CODEDIR}" ValidatePDFA_XXXX_ "${key}" "${pdffile}"
done
