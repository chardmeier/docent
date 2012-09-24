<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: modes14 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 5.7 -->
  <!-- Creator: Carmelo Montanez --><!-- Template004 in NIST suite -->
  <!-- Purpose: Test apply-templates for an attribute with a mode
       and moded matching template. -->

<xsl:template match="doc">
  <out>
    <xsl:apply-templates select="@attribute1" mode="mode1"/>
  </out>
</xsl:template>

<xsl:template match="@attribute1" mode="mode1">
  <xsl:value-of select="."/>
</xsl:template>

<xsl:template match="@attribute1">
  This test failed to execute properly.
</xsl:template>

</xsl:stylesheet>
