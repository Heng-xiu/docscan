<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>

<xsl:template match="log">

<xsl:for-each select="/log/logitem/fileanalysis[meta/jhove/profile/@pdfa1b='yes' and meta/verapdf/@pdfa1b='yes' and meta/pdfboxvalidator/@pdfa1b='yes' and meta/callaspdfapilot/@pdfa1b='yes']">
<xsl:variable name="filename" select="@filename" /> 
<xsl:value-of select="/log/logitem/uncompress[destination=$filename]/origin"/>
<xsl:text>
</xsl:text>
</xsl:for-each>

</xsl:template><!-- match="log" -->

</xsl:stylesheet>
