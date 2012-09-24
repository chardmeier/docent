<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: position19 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 2.4 -->
  <!-- Creator: Paul Dick -->
  <!-- Purpose: Test of positional indexing (shorthand) in select. Look for last item. -->

<xsl:template match="doc">
  <out>
    <xsl:value-of select="a[4]"/>
  </out>
</xsl:template>

</xsl:stylesheet>
