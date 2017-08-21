#!/usr/bin/env bash

export version=${version:-2.0.7}

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
