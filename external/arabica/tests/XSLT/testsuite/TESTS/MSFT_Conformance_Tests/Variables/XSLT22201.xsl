<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
    <xsl:param name="x">1</xsl:param>
    <xsl:template match="/">
        <root><xsl:value-of select="$x"/></root>
    </xsl:template>
</xsl:stylesheet>
