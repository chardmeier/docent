<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: NUMBERFORMAT13 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 12.3 -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test default decimal-format on pattern-only characters, negative number and one pattern. -->

<xsl:decimal-format digit="!" pattern-separator="\" />

<xsl:template match="doc">
  <out>
    <xsl:value-of select="format-number(-26931.4,'!!!,!!!.!!!')"/>
  </out>
</xsl:template>

</xsl:stylesheet>
