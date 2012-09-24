<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: idkey27 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 12.2 -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test whether blank keying (use) value works or is ignored. -->

<xsl:key name="mykey" match="div" use="title"/>

<xsl:template match="doc">
 <out>
  <xsl:value-of select="key('mykey','Introduction')/p"/><xsl:text>,</xsl:text>
  <xsl:value-of select="key('mykey','Stylesheet Structure')/p"/><xsl:text>,</xsl:text>
  <xsl:value-of select="key('mykey','Expressions')/p"/><xsl:text>,</xsl:text>
  <xsl:value-of select="key('mykey','')/p"/><xsl:text>,</xsl:text>
  <xsl:value-of select="key('mykey','Sorting')/p"/>
 </out>
</xsl:template>

</xsl:stylesheet>
