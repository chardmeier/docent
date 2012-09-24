<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: position52 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 4.1 -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test of count() on a set filtered by position. -->

<xsl:template match="doc">
  <out>
    <xsl:value-of select="count(*/z[2])"/>
  </out>
</xsl:template>

</xsl:stylesheet>
