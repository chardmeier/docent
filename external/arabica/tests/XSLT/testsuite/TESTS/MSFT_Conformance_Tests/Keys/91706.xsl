<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method="html" omit-xml-declaration="yes" indent="yes"/>

<!-- xsl:key wihtout a name attribute -->

<xsl:key match="p" use=".//term">
	<xsl:template match="/">
		<xsl:value-of select="text()"/>
	</xsl:template>
</xsl:key>

<xsl:template match="/">
    <TEST> should fail </TEST>
</xsl:template>

</xsl:stylesheet>
