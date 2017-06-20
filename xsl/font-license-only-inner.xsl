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
  Print number of PDF documents which have different combinations of font liceses,
  such as 'only open licenses' or 'only proprietary licenses'.
-->

<xsl:template match="log">

<!-- column header  -->
<xsl:text>license selection	file count</xsl:text>


<xsl:text>
total	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (meta/fonts/font[@oninnerpage='yes']/license/@type='open' or meta/fonts/font[@oninnerpage='yes']/license/@type='proprietary' or meta/fonts/font[@oninnerpage='yes']/license/@type='unknown')])" />


<xsl:text>
only open licenses	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (meta/fonts/font[@oninnerpage='yes']/license/@type='open' and not(meta/fonts/font[@oninnerpage='yes']/license/@type='proprietary') and not(meta/fonts/font[@oninnerpage='yes']/license/@type='unknown'))])" />

<xsl:text>
only proprietary licenses	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (not(meta/fonts/font[@oninnerpage='yes']/license/@type='open') and meta/fonts/font[@oninnerpage='yes']/license/@type='proprietary' and not(meta/fonts/font[@oninnerpage='yes']/license/@type='unknown'))])" />

<xsl:text>
only unknown licenses	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (not(meta/fonts/font[@oninnerpage='yes']/license/@type='open') and not(meta/fonts/font[@oninnerpage='yes']/license/@type='proprietary') and meta/fonts/font[@oninnerpage='yes']/license/@type='unknown')])" />



<xsl:text>
only open licenses or unknown licenses	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (meta/fonts/font[@oninnerpage='yes']/license/@type='open' and not(meta/fonts/font[@oninnerpage='yes']/license/@type='proprietary') and meta/fonts/font[@oninnerpage='yes']/license/@type='unknown')])" />

<xsl:text>
only proprietary licenses or unknown licenses	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (not(meta/fonts/font[@oninnerpage='yes']/license/@type='open') and meta/fonts/font[@oninnerpage='yes']/license/@type='proprietary' and meta/fonts/font[@oninnerpage='yes']/license/@type='unknown')])" />

<xsl:text>
only open licenses or proprietary licenses	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (meta/fonts/font[@oninnerpage='yes']/license/@type='open' and meta/fonts/font[@oninnerpage='yes']/license/@type='proprietary' and not(meta/fonts/font[@oninnerpage='yes']/license/@type='unknown'))])" />



<xsl:text>
proprietary licenses, open licenses, and unknown licenses	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (meta/fonts/font[@oninnerpage='yes']/license/@type='open' and meta/fonts/font[@oninnerpage='yes']/license/@type='proprietary' and meta/fonts/font[@oninnerpage='yes']/license/@type='unknown')])" />



<xsl:text>
</xsl:text>

</xsl:template><!-- match="log" -->

</xsl:stylesheet>
