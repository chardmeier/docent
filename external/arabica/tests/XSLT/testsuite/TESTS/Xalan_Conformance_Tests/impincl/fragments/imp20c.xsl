<?xml version="1.0"?> 
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<!-- FileName: imp20c -->
<!-- Purpose: Used indirectly by impincl20 -->

<xsl:template match="/">
  <xsl:text>WRONG Match on / in imp20c.xsl&#xa;</xsl:text>
  <xsl:apply-templates select="foo"/>
</xsl:template>

<xsl:template match="foo">
  <C>
    <xsl:apply-templates select="bar"/><!-- Should start at top of import tree -->
  </C>
</xsl:template>

<xsl:template match="bar">
  <xsl:text>match on bar in imp20c.xsl</xsl:text>
</xsl:template>

</xsl:stylesheet>
