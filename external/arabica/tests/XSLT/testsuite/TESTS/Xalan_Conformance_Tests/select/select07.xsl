<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: select07 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 7.1.2 Creating Elements -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Try to put out computed text without any tags.-->

<xsl:output method="text"/>

<xsl:template match="doc">
  <xsl:value-of select="inner"/>
</xsl:template>

</xsl:stylesheet>
