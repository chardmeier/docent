<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: impincl13 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 2.6 -->
  <!-- Creator: David Marston, from an idea by Norm Walsh -->
  <!-- Purpose: Show that included templates doing apply-imports *will* get includer's import tree. -->

<xsl:import href="fragments/i13imp.xsl"/>
<xsl:include href="fragments/i13incl.xsl"/>

<xsl:template match="/">
  <out><xsl:apply-templates select="doc/*" /></out>
</xsl:template>

<xsl:template match="author">
  MAIN author: <xsl:value-of select="."/>
  <xsl:apply-imports/>
</xsl:template>

<xsl:template match="title"><!-- This is last, so it wins out over the imports/includes -->
  MAIN title: <xsl:value-of select="."/>
</xsl:template>

</xsl:stylesheet>