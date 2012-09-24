<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: string01 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 4.2 String Functions  -->
  <!-- Creator: Paul Dick -->
  <!-- Purpose: Test of string-length() on literal string.  -->

<xsl:template match="/">
  <out>
    <xsl:value-of select="string-length('This is a test')"/>
  </out>
</xsl:template>

</xsl:stylesheet>
