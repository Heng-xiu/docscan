BEGIN {
	doprint=1
}

/<output>Jhove/ { doprint=0 }
/<issues[ >]/ { doprint=0 }

doprint==1 { print $0 }

/<\/issues>$/ { doprint=1 }
/<\/output>$/ { doprint=1 }
