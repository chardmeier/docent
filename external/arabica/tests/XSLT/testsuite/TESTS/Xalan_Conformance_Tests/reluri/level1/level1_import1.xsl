<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:import href="level2/level2_import1.xsl"/>

<xsl:template match="one-tag">
  From stylesheet level1_import1.xsl: <xsl:value-of select="self::node()"/>
  importing level2_import1.xsl
</xsl:template>

</xsl:stylesheet>