<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: reluri07 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 3.2 Base URI (Stylesheet import/Inclusion) -->
  <!-- Creator: Paul Dick -->
  <!-- Purpose: This test verifies correct URI resolution with relative URI's. --> 
  <!-- Case: include, include, import, include -->

<xsl:include href="level1/level1_include3.xsl"/>

<xsl:template match="root-tag">
 <out>
Case of: include, include, import, include
  Including level1_include3.xsl
  <xsl:apply-templates/>
 </out>
</xsl:template>

</xsl:stylesheet>