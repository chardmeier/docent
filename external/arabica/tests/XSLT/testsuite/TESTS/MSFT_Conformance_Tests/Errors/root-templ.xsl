<!-- Error Case -->
<xsl:template match="/" version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:for-each select="doc" order-by="name"/>
</xsl:template>
