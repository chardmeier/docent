<!-- Error Case -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

	<xsl:template match="/">

        <xsl:apply-templates>
            Hello
            <xsl:sort select="."/>
        </xsl:apply-templates>
	</xsl:template>

</xsl:stylesheet>
