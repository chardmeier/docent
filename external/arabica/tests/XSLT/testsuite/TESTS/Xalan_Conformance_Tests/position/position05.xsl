<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: position05 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 4.1 -->
  <!-- Creator: Paul Dick -->
  <!-- Purpose: Test of position() predicate on node-set from key(). Look for item 4. -->

<xsl:key name="k" match="a" use="@test"/>
<xsl:key name="k2" match="a" use="num"/>

<xsl:template match="doc">
  <out>
    <xsl:value-of select="key('k','true')[position()=4]/num"/>
  </out>
</xsl:template>

</xsl:stylesheet>
