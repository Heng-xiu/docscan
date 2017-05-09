<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>

<xsl:template match="log">

<!-- column header  -->
<xsl:text>filename	jhove-pdfa1b	verapdf-pdfa1b	pdfboxvalidator-pdfa1b	callaspdfapilot-pdfa1b	votes-pdfa1b</xsl:text>

<xsl:text>
total	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis/meta/jhove[profile/@pdfa1b='yes'])" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis/meta/verapdf[@pdfa1b='yes'])" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis/meta/pdfboxvalidator[@pdfa1b='yes'])" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis/meta/callaspdfapilot[@pdfa1b='yes'])" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis/meta[jhove/profile/@pdfa1b='yes' or verapdf/@pdfa1b='yes' or pdfboxvalidator/@pdfa1b='yes' or callaspdfapilot/@pdfa1b='yes'])" />
<xsl:text>
</xsl:text>

<xsl:for-each select="/log/logitem/fileanalysis">
<xsl:value-of select="@filename"/>
<xsl:text>	</xsl:text>
<xsl:choose>
 <xsl:when test="meta/jhove/profile/@pdfa1b">
  <xsl:value-of select="meta/jhove/profile/@pdfa1b" />
 </xsl:when>
 <!-- jHove does not always print a profile tag, so write 'no' if it does not exist -->
 <xsl:otherwise><xsl:text>no</xsl:text></xsl:otherwise>
</xsl:choose>
<xsl:text>	</xsl:text>
<xsl:value-of select="meta/verapdf/@pdfa1b" />
<xsl:text>	</xsl:text>
<xsl:value-of select="meta/pdfboxvalidator/@pdfa1b" />
<xsl:text>	</xsl:text>
<xsl:value-of select="meta/callaspdfapilot/@pdfa1b" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(meta/*[@pdfa1b='yes']) + count(meta/jhove/profile[@pdfa1b='yes'])" />
<xsl:text>
</xsl:text>
</xsl:for-each>

</xsl:template><!-- match="log" -->

</xsl:stylesheet>
