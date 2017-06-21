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
  Print the identified manufacturers and the number of documents that were generated
  with tools from each manufacturer.
-->

<xsl:key name="manufacturer" match="tool[@type='editor' or @type='producer']" use="name/@manufacturer" />

<xsl:template match="log">

<!-- column header  -->
<xsl:text>manufacturer	file count</xsl:text>

<xsl:text>
total	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/tools/tool[@type='editor' or @type='producer']/name/@manufacturer])" />
<xsl:text>
</xsl:text>

<xsl:for-each select="/log/logitem/fileanalysis[@status='ok']/meta/tools/tool[generate-id()=generate-id(key('manufacturer',name/@manufacturer))]">
<xsl:sort select="name/@manufacturer"/>
<xsl:variable name="cmpto" select="name/@manufacturer" />
<xsl:value-of select="name/@manufacturer" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/tools/tool[(@type='editor' or @type='producer') and name/@manufacturer=$cmpto]])" />
<xsl:text>
</xsl:text>
</xsl:for-each>

<xsl:text>SPECIAL:microsoft-AND-adobe	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/tools/tool/name/@manufacturer='adobe' and meta/tools/tool/name/@manufacturer='microsoft'])" />
<xsl:text>
</xsl:text>

<xsl:text>SPECIAL:microsoft-BUT-NO-adobe	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/tools/tool/name/@manufacturer='microsoft' and not(meta/tools/tool/name/@manufacturer='adobe')])" />
<xsl:text>
</xsl:text>

<xsl:text>SPECIAL:ONLY-microsoft	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/tools/tool[@type='editor']/name/@manufacturer='microsoft' and meta/tools/tool[@type='producer']/name/@manufacturer='microsoft'])" />
<xsl:text>
</xsl:text>

<xsl:text>SPECIAL:ONLY-adobe	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/tools/tool[@type='editor']/name/@manufacturer='adobe' and meta/tools/tool[@type='producer']/name/@manufacturer='adobe'])" />
<xsl:text>
</xsl:text>


</xsl:template><!-- match="log" -->

</xsl:stylesheet>
