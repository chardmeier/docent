<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: select02 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 12.4 -->
  <!-- Creator: Paul Dick -->
  <!-- Purpose: Test of current() function - just select it. -->

<xsl:template match="doc">
  <out>
    <xsl:value-of select="current()"/>
  </out>
</xsl:template>

</xsl:stylesheet>
