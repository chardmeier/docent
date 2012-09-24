<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: position49 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 2.4 -->
  <!-- Creator: Paul Dick -->
  <!-- Purpose: Test of positional indexing when used with key() in xsl:apply-templates. Look for last. -->

<xsl:key name="k" match="a" use="@test"/>

<xsl:template match="doc">
  <out>
    <xsl:apply-templates select="key('k','true')[4]/num"/>
  </out>
</xsl:template>

<xsl:template match="num">
  <B><xsl:value-of select="."/></B>
</xsl:template>

</xsl:stylesheet>
