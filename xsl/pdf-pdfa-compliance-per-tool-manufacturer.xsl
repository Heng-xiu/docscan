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
  Print the identified manufacturers, the number of documents that were generated
  with tools from each manufacturer, and the number of files that were deemed to be
  PDF/A-1b-compliant according to a quorum of votes from compliance testing tools.
-->

<xsl:key name="manufacturer" match="tool[@type='editor' or @type='producer']" use="name/@manufacturer" />

<xsl:template match="log">

<!-- column header  -->
<xsl:text>manufacturer	file count	compliant count</xsl:text>

<xsl:text>
total	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/tools/tool[@type='editor' or @type='producer']/name/@manufacturer])" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/tools/tool[@type='editor' or @type='producer']/name/@manufacturer and (count(meta/*[@pdfa1b='yes']) + count(meta/jhove/profile[@pdfa1b='yes']))>=3])" />
<xsl:text>
</xsl:text>

<xsl:for-each select="/log/logitem/fileanalysis[@status='ok']/meta/tools/tool[generate-id()=generate-id(key('manufacturer',name/@manufacturer))]">
<xsl:sort select="name/@manufacturer"/>
<xsl:variable name="cmpto" select="name/@manufacturer" />
<xsl:value-of select="name/@manufacturer" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/tools/tool[(@type='editor' or @type='producer') and name/@manufacturer=$cmpto]])" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/tools/tool[(@type='editor' or @type='producer') and name/@manufacturer=$cmpto] and (count(meta/*[@pdfa1b='yes']) + count(meta/jhove/profile[@pdfa1b='yes']))>=3])" />
<xsl:text>
</xsl:text>
</xsl:for-each>

</xsl:template><!-- match="log" -->

</xsl:stylesheet>
