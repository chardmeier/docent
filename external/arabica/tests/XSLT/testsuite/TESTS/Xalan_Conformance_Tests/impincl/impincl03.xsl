<?xml version="1.0"?> 
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: IMPINCL03 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 2.6.2 -->
  <!-- Purpose: Nest imports and includes so that there are two of each, import on top. -->

<xsl:import href="m.xsl"/>

<xsl:template match="/">
  <out>
    <xsl:value-of select="concat($mParam,$nParam,$oParam,$pParam)"/>
    <xsl:apply-templates/>
  </out>
</xsl:template>
 
<xsl:template match="title">
  Main stylesheet - title:
  <xsl:value-of select="."/>
</xsl:template>

</xsl:stylesheet>
