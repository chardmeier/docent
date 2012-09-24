<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: reluri09 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 12.1 Multiple Source Documents -->
  <!-- Creator: Paul Dick -->
  <!-- Purpose: Testing document() function with two arguments: string, node-set: 
       verifying that a relative URL specified in first argument is resolved based
	   on base URI of document in second argument nodeset. -->

<xsl:output indent="yes"/>

<xsl:template match="defaultcontent">
  <out>
    <xsl:copy-of select="document('level3/xreluri09a.xml',document('level1/level2/xreluri09b.xml'))"/>
  </out>
</xsl:template>

</xsl:stylesheet>
