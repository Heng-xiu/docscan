<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>

<!--
  Print the identified manufacturers and the number of documents that were generated
  with tools from each manufacturer.
-->

<xsl:key name="manufacturer" match="tool[@type='editor' or @type='producer']" use="name/@manufacturer" />

<xsl:template match="log">

<!-- column header  -->
<xsl:text>manufacturer	file count</xsl:text>

<xsl:text>
total	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/tools/tool[@type='editor' or @type='producer']/name/@manufacturer])" />
<xsl:text>
</xsl:text>

<xsl:for-each select="/log/logitem/fileanalysis[@status='ok']/meta/tools/tool[generate-id()=generate-id(key('manufacturer',name/@manufacturer))]">
<xsl:sort select="name/@manufacturer"/>
<xsl:variable name="cmpto" select="name/@manufacturer" />
<xsl:value-of select="name/@manufacturer" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/tools/tool[(@type='editor' or @type='producer') and name/@manufacturer=$cmpto]])" />
<xsl:text>
</xsl:text>
</xsl:for-each>

</xsl:template><!-- match="log" -->

</xsl:stylesheet>
