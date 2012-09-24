<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: select08 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 11.4 Top-level parameters -->
  <!-- Purpose: Test assignment of a node-set to a parameter, then use in select. -->
  <!-- Creator: David Marston -->

<xsl:param name="truenodes" select="/doc/*[@test='true']"/>

<xsl:template match="doc">
  <out>
    <xsl:apply-templates select="$truenodes"/>
  </out>
</xsl:template>

<xsl:template match="foo">
  <xsl:text>Got a foo node;</xsl:text>
</xsl:template>

<xsl:template match="wiz">
  <xsl:text>Got a wiz node;</xsl:text>
</xsl:template>

<xsl:template match="node()">
  <xsl:text>Got some other node;
</xsl:text>
</xsl:template>

</xsl:stylesheet>
