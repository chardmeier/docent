<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: NUMBERFORMAT19 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 12.3 -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test decimal-format declaration with a name. -->

<xsl:decimal-format name="myminus" minus-sign='_' />

<xsl:template match="doc">
  <out>
    <xsl:value-of select="format-number(-26931.4,'###,###.###','myminus')"/>
    <xsl:text>  </xsl:text>
    <xsl:value-of select="format-number(-42857.1,'###,###.###')"/>
  </out>
</xsl:template>

</xsl:stylesheet>
