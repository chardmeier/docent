<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: select42 -->
  <!-- Document: http://www.w3.org/TR/Xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 3.3 Node Sets -->
  <!-- Purpose: NodeSet union using absolute and relative location paths-->
  <!-- Creator: Carmelo Montanez --><!-- Expression002 in NIST suite -->

<xsl:template match="/">
  <out><xsl:text>
</xsl:text>
    <xsl:for-each select="doc">
      <xsl:apply-templates select = "sub1/child1|/doc/sub2/child2"/>
    </xsl:for-each>
  </out>
</xsl:template>

<xsl:template match="*">
  <xsl:value-of select="."/><xsl:text>
    </xsl:text>
</xsl:template>

</xsl:stylesheet>

