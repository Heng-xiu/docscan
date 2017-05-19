#!/usr/bin/env bash

export version=2.0.6

cd "$(dirname "${0}")"
./build.sh

jarfiles=$(ls -1 *.jar | xargs printf "%s:" ; echo ".")

for pdffile in "${@}" ; do
	test "${pdffile}" = "${pdffile/.pdf}" && continue
	java -cp "${jarfiles}" PdfBoxValidator "$(ls -1 ~/*.pdf | head -n 1)"
	exitcode=$?
	test ${exitcode} -eq 0 || exit ${exitcode}
done
