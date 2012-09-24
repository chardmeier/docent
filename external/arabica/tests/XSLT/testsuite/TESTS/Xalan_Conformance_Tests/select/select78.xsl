<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: select78 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 12.4 -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Simple test of current() in for-each. -->

<xsl:output method="xml" encoding="UTF-8"/>

<xsl:template match="doc">
  <out>
    <xsl:for-each select="foo">
      <xsl:text>&#10;</xsl:text>
      <match>
        <xsl:value-of select="current()/@name"/>
        <xsl:text> = </xsl:text>
        <xsl:value-of select="./@name"/>
      </match>
    </xsl:for-each>
  </out>
</xsl:template>

</xsl:stylesheet>
