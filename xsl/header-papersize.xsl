<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>

<xsl:template match="log">

<!-- column header  -->
<xsl:text>filename	width-mm	height-mm	format
</xsl:text>

<xsl:for-each select="/log/logitem/fileanalysis">
<xsl:value-of select="@filename" />
<xsl:text>	</xsl:text>
<xsl:value-of select="header/papersize/@width" />
<xsl:text>	</xsl:text>
<xsl:value-of select="header/papersize/@height" />
<xsl:text>	</xsl:text>
<xsl:value-of select="header/papersize" />
<xsl:text>
</xsl:text>
</xsl:for-each>

</xsl:template><!-- match="log" -->

</xsl:stylesheet>
