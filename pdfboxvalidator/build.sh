#!/usr/bin/env bash

version=${version:-2.0.6}

cd "$(dirname "${0}")"

for stem in fontbox pdfbox preflight xmpbox ; do
	fn="${stem}-${version}.jar"
	test -s "${fn}" && continue
	wget "http://www-eu.apache.org/dist/pdfbox/${version}/${fn}" || exit 1
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

test -s PdfBoxValidator.class || javac -cp "${jarfiles}" PdfBoxValidator.java
exit $?
