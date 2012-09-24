<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
    xmlns:ped="http://www.ped.com">

  <!-- FileName: namespace52 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 7.1.2 Creating Elements with xsl:element. -->
  <!-- Creator: Paul Dick -->
  <!-- Purpose: Use xsl:element with namespace in AVT, comes out as null string. -->

<xsl:template match="/">
 <out>
   <xsl:element name="{docs/b}" namespace="{docs/b/@href}"/>
 </out>
</xsl:template>

</xsl:stylesheet>
