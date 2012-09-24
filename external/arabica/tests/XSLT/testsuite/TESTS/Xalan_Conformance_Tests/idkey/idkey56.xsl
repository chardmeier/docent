<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: idkey56 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 4.1 -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test for id(list) and position() in apply-templates -->

<xsl:output method="xml" encoding="UTF-8"/>

<xsl:template match="/">
  <out>
    <xsl:text>
</xsl:text>
  <xsl:apply-templates select="id('c a d')" />
  </out>
</xsl:template>

<xsl:template match="a">
  <item>
    <xsl:value-of select="./@id"/>
    <xsl:text> is at position </xsl:text>
    <xsl:value-of select="position()"/>
  </item>
  <xsl:text>
</xsl:text>
</xsl:template>

</xsl:stylesheet>