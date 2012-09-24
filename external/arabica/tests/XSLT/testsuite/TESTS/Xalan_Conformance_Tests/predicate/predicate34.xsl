<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: PREDICATE34 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 3.4 -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test of implied conversion of node-set to number with an inequality relation. -->

<xsl:template match="doc">
  <out>
  <xsl:for-each select = "a[3 &lt;= following-sibling::*]">
    <xsl:value-of select="."/>
    <!-- True only when one of the following siblings contains a number greater than 3. -->
  </xsl:for-each>
  </out>
</xsl:template>

</xsl:stylesheet>
