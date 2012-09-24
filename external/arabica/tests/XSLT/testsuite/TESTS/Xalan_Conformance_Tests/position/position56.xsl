<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: position56 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 4.1 -->
  <!-- Creator: Paul Dick -->
  <!-- Purpose: Test of last() with node-set from key(). Long-form predicate. -->

<xsl:key name="k" match="a" use="@test"/>

<xsl:template match="doc">
  <out>
    <xsl:value-of select="key('k','true')[last()=position()]"/>
  </out>
</xsl:template>

</xsl:stylesheet>
