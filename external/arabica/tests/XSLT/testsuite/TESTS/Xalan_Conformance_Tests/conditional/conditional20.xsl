<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: conditional20 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 9.2 -->
  <!-- Purpose: Test two xsl:when elements using a variable in the test. -->

<xsl:template match="doc">
  <out>
    <xsl:variable name="level" select="position()"/>
    <xsl:choose>        
      <xsl:when test="$level=1">
        <xsl:text>Found the first one.</xsl:text>
      </xsl:when>
      <xsl:when test="$level=2">
        <xsl:text>Found the second one.</xsl:text>
      </xsl:when>
    </xsl:choose>
  </out>
</xsl:template>

</xsl:stylesheet>
