<?xml version='1.0'?>

<!--
    This file is part of DocScan.

    DocScan is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    DocScan is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DocScan.  If not, see <https://www.gnu.org/licenses/>.


    Copyright (2017) Thomas Fischer <thomas.fischer@his.se>, senior
    lecturer at University of Skövde, as part of the LIM-IT project.

-->

<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>

<!--
  Print all beautified font names, total number of variations used (regular, bold,
  italic, ...), and the number of files where this font is used at least once.
-->

<xsl:key name="fonts" match="font" use="beautified" />

<xsl:template match="log">

<!-- column header  -->
<xsl:text>font	license	occurrence count	file count</xsl:text>

<xsl:text>
total		</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok']/meta/fonts/font)" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fonts/font])" />

<xsl:text>
open		</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok']/meta/fonts/font[license/@type='open'])" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fonts/font/license/@type='open'])" />

<xsl:text>
proprietary		</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok']/meta/fonts/font[license/@type='proprietary'])" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fonts/font/license/@type='proprietary'])" />

<xsl:text>
unknown		</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok']/meta/fonts/font[license/@type='unknown'])" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fonts/font/license/@type='unknown'])" />


<xsl:for-each select="/log/logitem/fileanalysis[@status='ok']/meta/fonts/font[generate-id()=generate-id(key('fonts',beautified))]">
<xsl:sort select="beautified"/>
<xsl:variable name="cmpto" select="beautified" />
<xsl:text>
</xsl:text>
<xsl:value-of select="beautified" />
<xsl:text>	</xsl:text>
<xsl:value-of select="license/@type" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok']/meta/fonts/font[beautified=$cmpto])" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fonts/font/beautified=$cmpto])" />
</xsl:for-each>

<xsl:text>
</xsl:text>

</xsl:template><!-- match="log" -->

</xsl:stylesheet>
