<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:import href="level3/level3_import1.xsl"/>

<xsl:template match="two-tag">
  From stylesheet level2_import1.xsl: <xsl:value-of select="self::node()"/>
  importing level3_import1.xsl
</xsl:template>

</xsl:stylesheet>