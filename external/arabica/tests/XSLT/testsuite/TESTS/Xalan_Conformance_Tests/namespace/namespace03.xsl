<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
  xmlns:space="http://fictitious.com"
  version="1.0">

  <!-- FileName: namespace03 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 7.1.3 -->
  <!-- Creator: Paul Dick -->
  <!-- Purpose: Adding an attribute to an element replaces any existing
     attribute of that element with the same expanded-name. For attribute L,
     there is only a local name. Attribute Q has a namespace. -->

<xsl:template match="a">
  <out>The foo element....
    <foo xmlns:space="http://fictitious.com">
      <xsl:attribute name="L">loser1</xsl:attribute>
      <xsl:attribute name="space:Q">loser2</xsl:attribute>
      <xsl:attribute name="L">winner1</xsl:attribute>
      <xsl:attribute name="space:Q">winner2</xsl:attribute>
    </foo>
  </out>
</xsl:template>

</xsl:stylesheet>
