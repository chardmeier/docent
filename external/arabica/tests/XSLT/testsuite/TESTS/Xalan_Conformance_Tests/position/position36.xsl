<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: position36 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 4.1 -->
  <!-- Creator: Paul Dick -->
  <!-- Purpose: Test of position() in 2nd predicate. Look for first. -->

<xsl:template match="doc">
  <out>
    <xsl:value-of select="*[@test][position()=1]/num"/>
  </out>
</xsl:template>

</xsl:stylesheet>
