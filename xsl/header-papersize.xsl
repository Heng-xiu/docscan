<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>

<!--
  Print paper size for each PDF document (one number per line, one line per
  document).
-->

<xsl:template match="log">

<!-- column header  -->
<xsl:text>filename	width-mm	height-mm	format
</xsl:text>

<xsl:for-each select="/log/logitem/fileanalysis[@status='ok']">

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
