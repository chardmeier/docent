<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: math86 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 4.4 -->
  <!-- Creator: Paul Dick -->
  <!-- Purpose: Test of repeated use of * to multiply. -->

<xsl:template match="doc">
  <out>
    <xsl:value-of select="n1*n2*n3*n4"/><xsl:text>,</xsl:text>
    <xsl:value-of select="n1*n2*n3*n4*n5*n6*n7*n8*n9*n10"/>
  </out>
</xsl:template>

</xsl:stylesheet>
