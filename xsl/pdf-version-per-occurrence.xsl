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
  Print the unique PDF version numbers and the number of PDF documents that are of
  each version.
-->

<xsl:key name="pdfversions" match="fileformat" use="version" />

<xsl:template match="log">

<!-- column header  -->
<xsl:text>pdfversion	file count</xsl:text>

<xsl:text>
total	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fileformat/version])" />
<xsl:text>
</xsl:text>

<xsl:for-each select="/log/logitem/fileanalysis[@status='ok']/meta/fileformat[generate-id()=generate-id(key('pdfversions',version))]">
<xsl:sort select="version"/>
<xsl:variable name="cmpto" select="version" />
<xsl:value-of select="version" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fileformat/version=$cmpto])" />
<xsl:text>
</xsl:text>
</xsl:for-each>

</xsl:template><!-- match="log" -->

</xsl:stylesheet>
