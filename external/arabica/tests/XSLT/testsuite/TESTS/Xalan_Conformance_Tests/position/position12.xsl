<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: position12 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 4.1 -->
  <!-- Creator: Paul Dick -->
  <!-- Purpose: Test of position() function in value-of select. -->

<xsl:template match="doc">
  <out>
    <xsl:apply-templates select="p"/>
  </out>
</xsl:template>

<xsl:template match="p">
  <xsl:value-of select="position()=2"/><xsl:text>,</xsl:text>
</xsl:template>

</xsl:stylesheet>
