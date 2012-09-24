<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- FileName: variable54 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 11.2 -->
  <!-- Author: John Howard -->
  <!-- Purpose: test using parameter names with '.' in them -->
 
<xsl:output method="xml"/>

<xsl:template match="/">
  <out>
    <xsl:call-template name="foo">
      <xsl:with-param name="foo.bar" select="/data/*/datum/@value"/>
    </xsl:call-template>
  </out>
</xsl:template>

<xsl:template name="foo">
  <xsl:param name="foo.bar"/>

  <xsl:for-each select="$foo.bar">(<xsl:value-of select="position()"/>)</xsl:for-each>
</xsl:template>

</xsl:stylesheet>