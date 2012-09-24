<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: select09 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 11 Parameters -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test assignment of a node-set to a local parameter, then use in select. -->

<xsl:template match="doc">
  <xsl:param name="truenodes" select="*[@test='true']"/>
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
