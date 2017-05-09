<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>

<!--
  Print number of pages for each PDF document.
-->

<xsl:template match="log">

<!-- column header  -->
<xsl:text>filename	number of pages</xsl:text>

<xsl:for-each select="/log/logitem/fileanalysis[@status='ok']">
<xsl:sort select="header/num-pages"/>
<xsl:text>
</xsl:text>

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
<xsl:value-of select="header/num-pages" />
</xsl:for-each>

</xsl:template><!-- match="log" -->

</xsl:stylesheet>
