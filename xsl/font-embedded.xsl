<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>

<xsl:template match="log">

<!-- column header  -->
<xsl:text>embedded?	file count</xsl:text>


<xsl:text>
total	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[meta/font/@embedded])+count(/log/logitem/fileanalysis[meta/fonts/font/@embedded])" />


<xsl:text>
all embedded	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[(meta/font/@embedded='yes' and not(meta/font/@embedded='no')) or (meta/fonts/font/@embedded='yes' and not(meta/fonts/font/@embedded='no'))])" />

<xsl:text>
some embedded	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[(meta/font/@embedded='yes' and meta/font/@embedded='no') or (meta/fonts/font/@embedded='yes' and meta/fonts/font/@embedded='no')])" />


<xsl:text>
none embedded	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[(not(meta/font/@embedded='yes') and meta/font/@embedded='no') or (not(meta/fonts/font/@embedded='yes') and meta/fonts/font/@embedded='no')])" />


<xsl:text>
</xsl:text>

</xsl:template><!-- match="log" -->

</xsl:stylesheet>
