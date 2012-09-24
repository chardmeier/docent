<?xml version="1.0"?> 
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: MODES02 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 5.7 Modes -->
  <!-- Purpose: Test of moded template calling xsl:apply-templates 
       on another template. -->

<xsl:template match="/">
  <out>
    <xsl:apply-templates select="doc" mode="b"/>
  </out>
</xsl:template>
  
<xsl:template match="a" mode="a">
  <xsl:text>mode-a:</xsl:text><xsl:value-of select="."/>
</xsl:template>

<xsl:template match="a">
  <xsl:text>no-mode:</xsl:text><xsl:value-of select="."/>
</xsl:template>
 
<xsl:template match="doc" mode="b">
  mode-b: <xsl:apply-templates select="a"/>
</xsl:template>

</xsl:stylesheet>
