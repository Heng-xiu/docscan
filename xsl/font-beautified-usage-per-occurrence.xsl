<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>

<xsl:key name="fonts" match="font" use="beautified" />

<xsl:template match="log">

<!-- column header  -->
<xsl:text>font	license	occurrence count	file count</xsl:text>

<xsl:text>
total		</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis/meta/font)+count(/log/logitem/fileanalysis/meta/fonts/font)" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[meta/font])+count(/log/logitem/fileanalysis[meta/fonts/font])" />
<xsl:text>
</xsl:text>

<xsl:for-each select="/log/logitem/fileanalysis/meta//font[generate-id()=generate-id(key('fonts',beautified))]">
<xsl:variable name="cmpto" select="beautified" />
<xsl:value-of select="beautified" />
<xsl:text>	</xsl:text>
<xsl:value-of select="license/@type" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis/meta/font[beautified=$cmpto])+count(/log/logitem/fileanalysis/meta/fonts/font[beautified=$cmpto])" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[meta/font/beautified=$cmpto])+count(/log/logitem/fileanalysis[meta/fonts/font/beautified=$cmpto])" />
<xsl:text>
</xsl:text>
</xsl:for-each>

</xsl:template><!-- match="log" -->

</xsl:stylesheet>
