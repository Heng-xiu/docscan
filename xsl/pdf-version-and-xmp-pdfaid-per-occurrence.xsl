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
  Print version numbers and PDF/A compliance information
  as the PDF files state themselves.
-->

<xsl:key name="pdfversions" match="fileformat" use="version" />

<xsl:template match="log">

<!-- column header  -->
<xsl:text>combination	file count</xsl:text>

<xsl:text>
total	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fileformat/version])" />

<xsl:text>
PDF version pre-1.4	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (meta/fileformat/version='1.3' or meta/fileformat/version='1.2' or meta/fileformat/version='1.1' or meta/fileformat/version='1.0')])" />

<xsl:text>
PDF version post-1.4	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (meta/fileformat/version='1.5' or meta/fileformat/version='1.6' or meta/fileformat/version='1.7' or meta/fileformat/version='2.0')])" />


<xsl:text>
PDF version 1.4	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fileformat/version='1.4'])" />

<xsl:text>
PDF version 1.4 and PDF/A-1b	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fileformat/version='1.4' and meta/xmp/pdfconformance='PDF/A-1b'])" />

<xsl:text>
PDF version 1.4 and PDF/A-1b and valid 1+ out of 6	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fileformat/version='1.4' and meta/xmp/pdfconformance='PDF/A-1b' and count(meta[jhove/profile/@pdfa1b='yes' or verapdf/@pdfa1b='yes' or pdfboxvalidator/@pdfa1b='yes' or (adobepreflight/@pdfa1b='yes' or adobepreflight/@pdfa1a='yes') or qoppapdfpreflight/@pdfa1b='yes' or threeheightspdfvalidator/@pdfa1b='yes'])>=1])" />

<xsl:text>
PDF version 1.4 and PDF/A-1b and valid 2+ out of 6	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fileformat/version='1.4' and meta/xmp/pdfconformance='PDF/A-1b' and count(meta[jhove/profile/@pdfa1b='yes' or verapdf/@pdfa1b='yes' or pdfboxvalidator/@pdfa1b='yes' or (adobepreflight/@pdfa1b='yes' or adobepreflight/@pdfa1a='yes') or qoppapdfpreflight/@pdfa1b='yes' or threeheightspdfvalidator/@pdfa1b='yes'])>=2])" />

<xsl:text>
PDF version 1.4 and PDF/A-1b and valid 3+ out of 6	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fileformat/version='1.4' and meta/xmp/pdfconformance='PDF/A-1b' and count(meta[jhove/profile/@pdfa1b='yes' or verapdf/@pdfa1b='yes' or pdfboxvalidator/@pdfa1b='yes' or (adobepreflight/@pdfa1b='yes' or adobepreflight/@pdfa1a='yes') or qoppapdfpreflight/@pdfa1b='yes' or threeheightspdfvalidator/@pdfa1b='yes'])>=3])" />

<xsl:text>
PDF version 1.4 and PDF/A-1a	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fileformat/version='1.4' and meta/xmp/pdfconformance='PDF/A-1a'])" />


<xsl:text>
PDF/A-1b	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/xmp/pdfconformance='PDF/A-1b'])" />

<xsl:text>
PDF/A-1b but pre-1.4	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (meta/fileformat/version='1.3' or meta/fileformat/version='1.2' or meta/fileformat/version='1.1' or meta/fileformat/version='1.0') and meta/xmp/pdfconformance='PDF/A-1b'])" />

<xsl:text>
PDF/A-1b but post-1.4	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (meta/fileformat/version='1.5' or meta/fileformat/version='1.6' or meta/fileformat/version='1.7' or meta/fileformat/version='2.0') and meta/xmp/pdfconformance='PDF/A-1b'])" />


<xsl:text>
PDF/A-1a	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/xmp/pdfconformance='PDF/A-1a'])" />

<xsl:text>
PDF/A-1a but pre-1.4	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (meta/fileformat/version='1.3' or meta/fileformat/version='1.2' or meta/fileformat/version='1.1' or meta/fileformat/version='1.0') and meta/xmp/pdfconformance='PDF/A-1a'])" />

<xsl:text>
PDF/A-1a but post-1.4	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (meta/fileformat/version='1.5' or meta/fileformat/version='1.6' or meta/fileformat/version='1.7' or meta/fileformat/version='2.0') and meta/xmp/pdfconformance='PDF/A-1a'])" />


<xsl:text>
PDF version 1.7	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fileformat/version='1.7'])" />

<xsl:text>
PDF version 1.7 and PDF/A-2b	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fileformat/version='1.7' and meta/xmp/pdfconformance='PDF/A-2b'])" />

<xsl:text>
PDF version 1.7 and PDF/A-2a	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fileformat/version='1.7' and meta/xmp/pdfconformance='PDF/A-2a'])" />

<xsl:text>
PDF version 1.7 and PDF/A-2u	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fileformat/version='1.7' and meta/xmp/pdfconformance='PDF/A-2u'])" />


<xsl:text>
PDF/A-2b	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/xmp/pdfconformance='PDF/A-2b'])" />

<xsl:text>
PDF/A-2b but pre-1.7	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (meta/fileformat/version='1.6' or meta/fileformat/version='1.5' or meta/fileformat/version='1.4' or meta/fileformat/version='1.3' or meta/fileformat/version='1.2' or meta/fileformat/version='1.1' or meta/fileformat/version='1.0') and meta/xmp/pdfconformance='PDF/A-2b'])" />

<xsl:text>
PDF/A-2b but post-1.7	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fileformat/version='2.0' and meta/xmp/pdfconformance='PDF/A-2b'])" />

<xsl:text>
PDF/A-2a	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/xmp/pdfconformance='PDF/A-2a'])" />

<xsl:text>
PDF/A-2a but pre-1.7	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (meta/fileformat/version='1.6' or meta/fileformat/version='1.5' or meta/fileformat/version='1.4' or meta/fileformat/version='1.3' or meta/fileformat/version='1.2' or meta/fileformat/version='1.1' or meta/fileformat/version='1.0') and meta/xmp/pdfconformance='PDF/A-2a'])" />

<xsl:text>
PDF/A-2a but post-1.7	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fileformat/version='2.0' and meta/xmp/pdfconformance='PDF/A-2a'])" />

<xsl:text>
PDF/A-2u	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/xmp/pdfconformance='PDF/A-2u'])" />

<xsl:text>
PDF/A-2u but pre-1.7	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (meta/fileformat/version='1.6' or meta/fileformat/version='1.5' or meta/fileformat/version='1.4' or meta/fileformat/version='1.3' or meta/fileformat/version='1.2' or meta/fileformat/version='1.1' or meta/fileformat/version='1.0') and meta/xmp/pdfconformance='PDF/A-2u'])" />

<xsl:text>
PDF/A-2u but post-1.7	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/fileformat/version='2.0' and meta/xmp/pdfconformance='PDF/A-2u'])" />

<xsl:text>
Valid 1+ out of 6	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and count(meta[jhove/profile/@pdfa1b='yes' or verapdf/@pdfa1b='yes' or pdfboxvalidator/@pdfa1b='yes' or (adobepreflight/@pdfa1b='yes' or adobepreflight/@pdfa1a='yes') or qoppapdfpreflight/@pdfa1b='yes' or threeheightspdfvalidator/@pdfa1b='yes'])>=1])" />

<xsl:text>
Valid 2+ out of 6	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and count(meta[jhove/profile/@pdfa1b='yes' or verapdf/@pdfa1b='yes' or pdfboxvalidator/@pdfa1b='yes' or (adobepreflight/@pdfa1b='yes' or adobepreflight/@pdfa1a='yes') or qoppapdfpreflight/@pdfa1b='yes' or threeheightspdfvalidator/@pdfa1b='yes'])>=2])" />

<xsl:text>
Valid 3+ out of 6	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and count(meta[jhove/profile/@pdfa1b='yes' or verapdf/@pdfa1b='yes' or pdfboxvalidator/@pdfa1b='yes' or (adobepreflight/@pdfa1b='yes' or adobepreflight/@pdfa1a='yes') or qoppapdfpreflight/@pdfa1b='yes' or threeheightspdfvalidator/@pdfa1b='yes'])>=3])" />

<xsl:text>
</xsl:text>

</xsl:template><!-- match="log" -->

</xsl:stylesheet>
