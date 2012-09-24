<?xml version="1.0"?> 
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: position100 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 2.2 -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test count() starting on a text node and going upward. -->

<xsl:output method="xml" indent="no" encoding="UTF-8"/>

<xsl:template match="/">
  <out>
    <xsl:for-each select="//center/text()[1]">
      <xsl:value-of select="count(ancestor-or-self::node())"/><xsl:text> nodes on this axis:
</xsl:text>
      <xsl:apply-templates select="ancestor-or-self::node()" mode="census"/>
    </xsl:for-each>
  </out>
</xsl:template>

<xsl:template match="/" mode="census">
  <xsl:text>Root Node
</xsl:text>
</xsl:template>

<xsl:template match="*" mode="census">
  <xsl:text>E: </xsl:text><xsl:value-of select="name(.)"/><xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="@*" mode="census">
  <xsl:text>A: </xsl:text><xsl:value-of select="name(.)"/><xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="text()" mode="census">
  <xsl:text>T: </xsl:text><xsl:value-of select="."/><xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="comment()|processing-instruction()" mode="census">
  <xsl:text>ERROR! </xsl:text><xsl:value-of select="."/><xsl:text>
</xsl:text>
</xsl:template>

</xsl:stylesheet>
