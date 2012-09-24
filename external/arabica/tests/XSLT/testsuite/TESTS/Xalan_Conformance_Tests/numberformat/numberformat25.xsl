<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: NUMBERFORMAT25 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 12.3 -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Create a conflict in the use of the '.' character. -->

<xsl:decimal-format decimal-separator="!" grouping-separator="!" />

<xsl:template match="doc">
  <out>
    <xsl:value-of select="format-number(931.4857,'###!###!###')"/>
  </out>
</xsl:template>

</xsl:stylesheet>
