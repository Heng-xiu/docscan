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
  Track and count the 'votes' for PDF/A-1a compliance by external tools:
  veraPDF, PDFBox, Adobe Preflight, Qoppa PDFPreflight, PDF Tool's 3-Heights Validator and jHove.
  Print both summary and a per-file analysis
-->

<xsl:template match="log">

<!-- column header  -->
<xsl:text>filename	jhove-pdfa1a	verapdf-pdfa1a	pdfboxvalidator-pdfa1a	adobe-preflight	qoppa-pdfpreflight	pdftools-3heights	votes-pdfa1a</xsl:text>

<xsl:text>
total	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok']/meta/jhove[profile/@pdfa1a='yes'])" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok']/meta/verapdf[@pdfa1a='yes'])" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok']/meta/pdfboxvalidator[@pdfa1a='yes'])" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok']/meta/adobepreflight[@pdfa1a='yes'])" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok']/meta/qoppapdfpreflight[@pdfa1a='yes'])" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok']/meta/threeheightspdfvalidator[@pdfa1a='yes'])" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok']/meta[jhove/profile/@pdfa1a='yes' or verapdf/@pdfa1a='yes' or pdfboxvalidator/@pdfa1a='yes' or adobepreflight/@pdfa1a='yes' or qoppapdfpreflight/@pdfa1a='yes' or threeheightspdfvalidator/@pdfa1a='yes'])" />
<xsl:text>
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
<xsl:choose>
 <xsl:when test="meta/jhove/profile/@pdfa1a">
  <xsl:value-of select="meta/jhove/profile/@pdfa1a" />
 </xsl:when>
 <xsl:when test="meta/jhove/profile">
  <!-- jHove does not always print a profile tag, so write 'no' if it does not exist -->
  <xsl:text>no</xsl:text>
 </xsl:when>
 <!-- jHove is not always run -->
 <xsl:otherwise><xsl:text>no-data</xsl:text></xsl:otherwise>
</xsl:choose>
<xsl:text>	</xsl:text>
<xsl:choose>
 <xsl:when test="meta/verapdf/@pdfa1a">
  <xsl:value-of select="meta/verapdf/@pdfa1a" />
 </xsl:when>
 <!-- veraPDF is not always run -->
 <xsl:otherwise><xsl:text>no-data</xsl:text></xsl:otherwise>
</xsl:choose>
<xsl:text>	</xsl:text>
<xsl:choose>
 <xsl:when test="meta/pdfboxvalidator/@pdfa1a">
  <xsl:value-of select="meta/pdfboxvalidator/@pdfa1a" />
 </xsl:when>
 <!-- Apache PDFBox Validator is not always run -->
 <xsl:otherwise><xsl:text>no-data</xsl:text></xsl:otherwise>
</xsl:choose>
<xsl:text>	</xsl:text>
<xsl:choose>
 <xsl:when test="meta/adobepreflight/@pdfa1a">
  <xsl:value-of select="meta/adobepreflight/@pdfa1a" />
 </xsl:when>
 <!-- Adobe Preflight is not always run -->
 <xsl:otherwise><xsl:text>no-data</xsl:text></xsl:otherwise>
</xsl:choose>
<xsl:text>	</xsl:text>
<xsl:choose>
 <xsl:when test="meta/qoppapdfpreflight/@pdfa1a">
  <xsl:value-of select="meta/qoppapdfpreflight/@pdfa1a" />
 </xsl:when>
 <!-- Qoppa PDFPreflight is not always run -->
 <xsl:otherwise><xsl:text>no-data</xsl:text></xsl:otherwise>
</xsl:choose>
<xsl:text>	</xsl:text>
<xsl:choose>
 <xsl:when test="meta/threeheightspdfvalidator/@pdfa1a">
  <xsl:value-of select="meta/threeheightspdfvalidator/@pdfa1a" />
 </xsl:when>
 <!-- PDF Tool's 3-Heights PDF Validator is not always run -->
 <xsl:otherwise><xsl:text>no-data</xsl:text></xsl:otherwise>
</xsl:choose>
<xsl:text>	</xsl:text>
<xsl:value-of select="count(meta/*[@pdfa1a='yes']) + count(meta/jhove/profile[@pdfa1a='yes'])" />
<xsl:text>
</xsl:text>
</xsl:for-each>

</xsl:template><!-- match="log" -->

</xsl:stylesheet>
