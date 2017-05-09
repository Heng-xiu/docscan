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
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (meta/fonts/font/license/@type='open' or meta/fonts/font/license/@type='proprietary' or meta/fonts/font/license/@type='unknown')])" />


<xsl:text>
only open licenses	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (meta/fonts/font/license/@type='open' and not(meta/fonts/font/license/@type='proprietary') and not(meta/fonts/font/license/@type='unknown'))])" />

<xsl:text>
only proprietary licenses	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (not(meta/fonts/font/license/@type='open') and meta/fonts/font/license/@type='proprietary' and not(meta/fonts/font/license/@type='unknown'))])" />

<xsl:text>
only unknown licenses	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (not(meta/fonts/font/license/@type='open') and not(meta/fonts/font/license/@type='proprietary') and meta/fonts/font/license/@type='unknown')])" />



<xsl:text>
only open licenses or unknown licenses	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (meta/fonts/font/license/@type='open' and not(meta/fonts/font/license/@type='proprietary') and meta/fonts/font/license/@type='unknown')])" />

<xsl:text>
only proprietary licenses or unknown licenses	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (not(meta/fonts/font/license/@type='open') and meta/fonts/font/license/@type='proprietary' and meta/fonts/font/license/@type='unknown')])" />

<xsl:text>
only open licenses or proprietary licenses	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (meta/fonts/font/license/@type='open' and meta/fonts/font/license/@type='proprietary' and not(meta/fonts/font/license/@type='unknown'))])" />



<xsl:text>
proprietary licenses, open licenses, and unknown licenses	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and (meta/fonts/font/license/@type='open' and meta/fonts/font/license/@type='proprietary' and meta/fonts/font/license/@type='unknown')])" />



<xsl:text>
</xsl:text>

</xsl:template><!-- match="log" -->

</xsl:stylesheet>