<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: copy25 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 7.5 -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Accumulate attributes from several places in the source. -->

<xsl:output method="xml" encoding="UTF-8" />

<xsl:template match="/">
  <out>
    <a>
      <!-- first, get all attribute nodes under the 'a' node in the source -->
      <xsl:for-each select="main/a/descendant-or-self::*/@*">
        <xsl:copy/>
      </xsl:for-each>
      <!-- next, get an attribute node from elsewhere -->
      <xsl:for-each select="main/size[@for='d']">
        <xsl:apply-templates select="@h | @w"/>
      </xsl:for-each>
    </a>
  </out>
</xsl:template>

<xsl:template match="@*">
  <xsl:copy/>
</xsl:template>

</xsl:stylesheet>
