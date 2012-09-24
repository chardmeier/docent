<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: node14 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 5.1 Root node -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test for access to PIs hanging off the root. -->

<xsl:template match="/">
  <out>
    <xsl:text>
Root-level processing instructions:</xsl:text>
    <xsl:for-each select="processing-instruction()">
      <xsl:text>
</xsl:text><xsl:value-of select="name()"/>
    </xsl:for-each>
    <xsl:text>
</xsl:text>
  </out>
</xsl:template>

</xsl:stylesheet>
