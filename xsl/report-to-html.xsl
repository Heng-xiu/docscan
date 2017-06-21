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
<xsl:output method="html" omit-xml-declaration="yes" indent="yes" encoding="UTF-8"/>

<!--
  Generates a HTML report on a file analysis.
-->

<xsl:template match="fileanalysis">
<li style="padding-bottom: 1em; padding-top: 1em;"><span style="font-size: 150%; background: rgba(191,191,191,.5)"><code><!-- either use file's name or, if decompressed, its original filename -->
<xsl:variable name="filename" select="@filename" />
<xsl:choose>
  <xsl:when test="/log/logitem/uncompress[destination=$filename]/origin">
    <xsl:value-of select="/log/logitem/uncompress[destination=$filename]/origin"/>
  </xsl:when>
  <xsl:otherwise>
    <xsl:value-of select="$filename"/>
  </xsl:otherwise>
</xsl:choose></code></span><br/>
<table><tbody>
<tr><td>Status:</td><td style="font-weight: bold;"><xsl:value-of select="@status" /></td></tr>
<xsl:if test="jhove">
<tr><td>Size:</td><td style="font-weight: bold;"><xsl:value-of select="jhove/meta/file/@size" />&#160;Bytes</td></tr>
<tr><td>Area:</td><td style="font-weight: bold;"><xsl:value-of select="jhove/meta/rect/@width" />&#215;<xsl:value-of select="jhove/meta/rect/@height" /></td></tr>
</xsl:if>
<xsl:if test="meta/fileformat/mimetype='application/pdf'">
<tr><td>Size:</td><td style="font-weight: bold;"><xsl:value-of select="meta/file/@size" />&#160;Bytes</td></tr>
<tr><td>PDF/A-1b compliant:</td><td style="font-weight: bold;">
<xsl:choose>
  <xsl:when test="(count(meta[jhove/profile/@pdfa1b='yes'])+count(meta[verapdf/@pdfa1b='yes'])+count(meta[pdfboxvalidator/@pdfa1b='yes'])+count(meta[callaspdfapilot/@pdfa1b='yes'])) = 0">
    No
  </xsl:when>
  <xsl:when test="(count(meta[jhove/profile/@pdfa1b='yes'])+count(meta[verapdf/@pdfa1b='yes'])+count(meta[pdfboxvalidator/@pdfa1b='yes'])+count(meta[callaspdfapilot/@pdfa1b='yes'])) = 1">
    Unlikely
  </xsl:when>
  <xsl:when test="(count(meta[jhove/profile/@pdfa1b='yes'])+count(meta[verapdf/@pdfa1b='yes'])+count(meta[pdfboxvalidator/@pdfa1b='yes'])+count(meta[callaspdfapilot/@pdfa1b='yes'])) = 2">
    Possibly
  </xsl:when>
  <xsl:when test="(count(meta[jhove/profile/@pdfa1b='yes'])+count(meta[verapdf/@pdfa1b='yes'])+count(meta[pdfboxvalidator/@pdfa1b='yes'])+count(meta[callaspdfapilot/@pdfa1b='yes'])) > 2">
    Most likely
  </xsl:when>
  <xsl:otherwise>
    Unknown
  </xsl:otherwise>
</xsl:choose>
(<xsl:value-of select="count(meta[jhove/profile/@pdfa1b='yes'])+count(meta[verapdf/@pdfa1b='yes'])+count(meta[pdfboxvalidator/@pdfa1b='yes'])+count(meta[callaspdfapilot/@pdfa1b='yes'])"/>&#160;votes)
</td></tr>
<tr><td>PDF/A-1a compliant:</td><td style="font-weight: bold;">
<xsl:choose>
  <xsl:when test="(count(meta[jhove/profile/@pdfa1a='yes'])+count(meta[verapdf/@pdfa1a='yes'])+count(meta[pdfboxvalidator/@pdfa1a='yes'])+count(meta[callaspdfapilot/@pdfa1a='yes'])) = 0">
    No
  </xsl:when>
  <xsl:when test="(count(meta[jhove/profile/@pdfa1a='yes'])+count(meta[verapdf/@pdfa1a='yes'])+count(meta[pdfboxvalidator/@pdfa1a='yes'])+count(meta[callaspdfapilot/@pdfa1a='yes'])) = 1">
    Unlikely
  </xsl:when>
  <xsl:when test="(count(meta[jhove/profile/@pdfa1a='yes'])+count(meta[verapdf/@pdfa1a='yes'])+count(meta[pdfboxvalidator/@pdfa1a='yes'])+count(meta[callaspdfapilot/@pdfa1a='yes'])) = 2">
    Possibly
  </xsl:when>
  <xsl:when test="(count(meta[jhove/profile/@pdfa1a='yes'])+count(meta[verapdf/@pdfa1a='yes'])+count(meta[pdfboxvalidator/@pdfa1a='yes'])+count(meta[callaspdfapilot/@pdfa1a='yes'])) > 2">
    Most likely
  </xsl:when>
  <xsl:otherwise>
    Unknown
  </xsl:otherwise>
</xsl:choose>
(<xsl:value-of select="count(meta[jhove/profile/@pdfa1a='yes'])+count(meta[verapdf/@pdfa1a='yes'])+count(meta[pdfboxvalidator/@pdfa1a='yes'])+count(meta[callaspdfapilot/@pdfa1a='yes'])"/>&#160;votes)
</td></tr>
</xsl:if>
</tbody></table>
</li>
</xsl:template><!-- match="log" -->

<xsl:template match="log">
<html>
<head>
<meta charset="UTF-8" />
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
</head>
<body>
<ol>
<xsl:apply-templates select="logitem/fileanalysis"/>
</ol>
</body></html>
</xsl:template><!-- match="log" -->

</xsl:stylesheet>
