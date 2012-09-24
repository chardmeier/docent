<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: variable44 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 11.5 -->
  <!-- Purpose: Set a variable inside a template based on variable defined earlier in that template. -->
  <!-- Author: David Marston -->

<xsl:template match="doc">
  <out>
    <xsl:variable name="a" select="'first'" /><!-- Two sets of quotes make it a string -->
    <xsl:variable name="b" select="$a" />
    <xsl:value-of select="$b" />
  </out>
</xsl:template>

</xsl:stylesheet>