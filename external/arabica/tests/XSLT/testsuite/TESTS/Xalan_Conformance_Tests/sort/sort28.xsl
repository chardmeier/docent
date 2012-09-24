<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: SORT28 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 10 -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test for xsl:sort on apply-templates without select, numeric sort. -->

<xsl:template match="main">
  <out>
    <xsl:apply-templates>
      <xsl:sort select="@size" data-type="number"/>
    </xsl:apply-templates>
  </out>
</xsl:template>

<xsl:template match="sub | text()">
  <xsl:value-of select="."/><xsl:text>-</xsl:text>
</xsl:template>

</xsl:stylesheet>
