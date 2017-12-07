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

<xsl:stylesheet version='1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>

<!--
  Print the identified manufacturers and the number of documents that were generated
  with tools from each manufacturer.
-->

<xsl:template match="log">

<!-- column header  -->
<xsl:text>filename	editor string	editor manufacturer	editor product	producer string	producer manufacturer	producer product
</xsl:text>

<xsl:for-each select="/log/logitem/fileanalysis[@status='ok']">

<!-- either use file's name or, if decompressed, its original filename -->
<xsl:variable name="filename" select="@filename" />
<xsl:choose>
  <xsl:when test="/log/logitem/uncompress[destination=$filename]/origin">
    <xsl:value-of select="/log/logitem/uncompress[destination=$filename]/origin"/>
  </xsl:when>
  <xsl:otherwise>
    <xsl:value-of select="$filename"/>
  </xsl:otherwise>
</xsl:choose>

<xsl:text>	</xsl:text>

<xsl:value-of select="normalize-space(meta/tools/tool[@type='editor'])"/>
<xsl:text>	</xsl:text>
<xsl:value-of select="normalize-space(meta/tools/tool[@type='editor']/name/@manufacturer)"/>
<xsl:text>	</xsl:text>
<xsl:value-of select="normalize-space(meta/tools/tool[@type='editor']/name/@product)"/>
<xsl:text>	</xsl:text>
<xsl:value-of select="normalize-space(meta/tools/tool[@type='producer'])"/>
<xsl:text>	</xsl:text>
<xsl:value-of select="normalize-space(meta/tools/tool[@type='producer']/name/@manufacturer)"/>
<xsl:text>	</xsl:text>
<xsl:value-of select="normalize-space(meta/tools/tool[@type='producer']/name/@product)"/>
<xsl:text>
</xsl:text>

</xsl:for-each>

</xsl:template><!-- match="log" -->

</xsl:stylesheet>
