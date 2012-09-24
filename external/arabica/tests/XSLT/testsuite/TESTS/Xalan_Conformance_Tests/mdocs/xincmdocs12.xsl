<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
                xmlns:ped="http://ped.com"
                exclude-result-prefixes="xsl ped">

  <!-- Purpose: Included by mdocs12 -->

<xsl:template match="test">
  <xsl:copy-of select="document('')/xsl:stylesheet/ped:test[@attrib='yeha']"/>
</xsl:template>

<ped:test attrib="yeha">YEE-HA</ped:test>
<ped:test attrib="haye">Wrong item from sub</ped:test>

</xsl:stylesheet>