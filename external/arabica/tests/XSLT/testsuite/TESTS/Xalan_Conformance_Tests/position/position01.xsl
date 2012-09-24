<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: position01 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 4.1 -->
  <!-- Creator: Paul Dick -->
  <!-- Purpose: Test of position() function in value-of select. Look for 1. -->

<xsl:template match="doc">
  <out>
    <xsl:value-of select="position()=1"/>
  </out>
</xsl:template>

</xsl:stylesheet>
