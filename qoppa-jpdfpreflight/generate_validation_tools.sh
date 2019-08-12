#!/usr/bin/env bash

chmod a-x ValidatePDFA_XXXX_.sh ValidatePDFA_XXXX_.java || exit 1
for version in 1b 1a 2b 2u 3b ; do
	qoppaversion=${version:0:1}"_"
	case ${version:1:1} in
	"a") qoppaversion="${qoppaversion}A" ;;
	"u") qoppaversion="${qoppaversion}U" ;;
	*) qoppaversion="${qoppaversion}B" ;;
	esac
	sed -e 's!_XXXX_!'"${version}"'!g' <ValidatePDFA_XXXX_.sh >ValidatePDFA${version}.sh || exit 1
	sed -e 's!_XXXX_!'"${version}"'!g;s!_YYYY_!'"${qoppaversion}"'!g' <ValidatePDFA_XXXX_.java >ValidatePDFA${version}.java || exit 1
	chmod a+x ValidatePDFA${version}.sh || exit 1
done
