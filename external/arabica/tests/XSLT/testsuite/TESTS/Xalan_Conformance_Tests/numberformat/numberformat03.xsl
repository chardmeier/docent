<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: NUMBERFORMAT03 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 12.3 -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test of format-number on a negative number. -->

<xsl:template match="doc">
  <out>
    <xsl:value-of select="format-number(2792.14*(-36.58),'000,000.000###')"/>
  </out>
</xsl:template>

</xsl:stylesheet>
