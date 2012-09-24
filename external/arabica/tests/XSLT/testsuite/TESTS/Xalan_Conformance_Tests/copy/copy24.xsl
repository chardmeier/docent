<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: copy24 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 2.3 -->
  <!-- Creator: Myriam Midy -->
  <!-- Purpose: Test for processing-instruction() node-test in copy-of. -->

<!-- should say "Found-pi,,Found-pi" -->

<xsl:template match="/">
  <out>
    <xsl:copy-of select="./processing-instruction('a-pi')"/><xsl:text>...
</xsl:text>
    <xsl:copy-of select="./processing-instruction()"/><xsl:text>...
</xsl:text>
    <xsl:copy-of select="*"/>
  </out>
</xsl:template>

</xsl:stylesheet>
