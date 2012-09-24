<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: MATCH18 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 5.2 -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test of predicate with two conditions, one positional. -->

<xsl:template match="doc">
  <out>
    <xsl:apply-templates/>
  </out>
</xsl:template>

<xsl:template match="foo[position()=2 and @att2='ok']">
  <xsl:value-of select="@spot"/>
  <xsl:text> </xsl:text>
</xsl:template>

<xsl:template match="text()"/><!-- suppress built-in template for text -->

</xsl:stylesheet>
