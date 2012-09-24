<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: impincl01 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 2.6.1 Stylesheet Inclusion -->
  <!-- Purpose: Test of basic Import & Include functionality. -->

<xsl:import href="i.xsl"/>
<xsl:include href="j.xsl"/>

<xsl:template match="root-tag">
 <out>
  <xsl:apply-templates/>
 </out>
</xsl:template>

</xsl:stylesheet>
