<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>

<!--
  Extract the filenames of all PDF documents were all four PDF/A compliance checkers
  agree upon that those files are PDF/A-1b-compliant.
-->

<xsl:template match="log">

<xsl:for-each select="/log/logitem/fileanalysis[@status='ok' and meta/jhove/profile/@pdfa1b='yes' and meta/verapdf/@pdfa1b='yes' and meta/pdfboxvalidator/@pdfa1b='yes' and meta/callaspdfapilot/@pdfa1b='yes']">
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
<xsl:text>
</xsl:text>
</xsl:for-each>

</xsl:template><!-- match="log" -->

</xsl:stylesheet>
