<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: boolean38 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 7.6.1 -->
  <!-- Purpose: Display a problem expression -->

<xsl:template match="doc">
  <out>
    <xsl:value-of select="0 div 0"/>
  </out>
</xsl:template>

</xsl:stylesheet>
