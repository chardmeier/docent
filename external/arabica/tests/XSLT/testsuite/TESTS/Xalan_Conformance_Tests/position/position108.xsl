<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: position108 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 4.1 -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test for last() on 'following-sibling::' axis. -->

<xsl:output method="xml" encoding="UTF-8" indent="no"/>

<xsl:template match="/">
  <out>
    <xsl:apply-templates select="/doc/north"/>
  </out>
</xsl:template>

<xsl:template match="north">
  <xsl:text>last following: </xsl:text>
  <xsl:value-of select="name(following-sibling::*[last()])"/>
</xsl:template>

</xsl:stylesheet>