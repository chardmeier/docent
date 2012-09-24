<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: select72 -->
  <!-- Document: http://www.w3.org/TR/Xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 3.3 Node Sets -->
  <!-- Purpose: NodeSet union using two copies of same node, and variables -->
  <!-- Creator: David Marston -->
  <!-- This could be used to establish that the variable is of the node-set type,
     as opposed to string or one-node RTF, where the count would go up. -->

<xsl:template match="/">
  <out>
    <xsl:variable name = "var1" select = "doc/sub1/child1" />
    <xsl:text>
Count of node-set: </xsl:text>
    <xsl:value-of select = "count($var1)"/>
    <xsl:variable name = "var2" select = "$var1|$var1" />
    <xsl:text>
Count of union: </xsl:text>
    <xsl:value-of select = "count($var2)"/>
  </out>
</xsl:template>

</xsl:stylesheet>