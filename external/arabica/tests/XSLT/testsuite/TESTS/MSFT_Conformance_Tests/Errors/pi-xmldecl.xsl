<!-- Error Case -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:template match="/">
		<xsl:processing-instruction name="xml">version='1.0'</xsl:processing-instruction>
	</xsl:template>
</xsl:stylesheet>
