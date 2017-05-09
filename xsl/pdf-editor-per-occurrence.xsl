<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>

<!--
  Print the unique PDF editors and the number of PDF documents that are tagged by
  this PDF editor. Include the guessed editor's manufacturer as well.
-->

<xsl:key name="pdfeditors" match="tool[@type='editor']" use="name" />

<xsl:template match="log">

<!-- column header  -->
<xsl:text>pdfeditor	manufacturer	file count</xsl:text>

<xsl:text>
total	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok' and meta/tools/tool/@type='editor'])" />
<xsl:text>
</xsl:text>

<xsl:for-each select="/log/logitem/fileanalysis[@status='ok']/meta/tools/tool[generate-id()=generate-id(key('pdfeditors',name))]">
<xsl:sort select="name/@manufacturer"/>
<xsl:variable name="cmpto" select="name" />
<xsl:value-of select="name" />
<xsl:text>	</xsl:text>
<xsl:value-of select="name/@manufacturer" />
<xsl:text>	</xsl:text>
<xsl:value-of select="count(/log/logitem/fileanalysis[@status='ok']/meta/tool[@type='editor' and name=$cmpto])+count(/log/logitem/fileanalysis[@status='ok']/meta/tools/tool[@type='editor' and name=$cmpto])" />
<xsl:text>
</xsl:text>
</xsl:for-each>

</xsl:template><!-- match="log" -->

</xsl:stylesheet>
