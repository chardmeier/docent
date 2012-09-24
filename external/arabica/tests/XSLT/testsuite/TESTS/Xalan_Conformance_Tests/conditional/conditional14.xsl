<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: conditional14 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 9.1 -->
  <!-- Purpose: Test xsl:if with test of subelement value. -->

<xsl:template match="Family">
  <out>
    <xsl:apply-templates select="Child"/>
  </out>
</xsl:template>

<xsl:template match="Child">
  <xsl:if test='Personal_Information/Sex[.="Male"]' >
    <xsl:value-of select="./Name" />, he is <xsl:value-of select="Personal_Information/Age"/><xsl:text> years old.&#10;</xsl:text>
  </xsl:if>
  <xsl:if test='Personal_Information/Sex[.="Female"]' >
    <xsl:value-of select="./Name" />, she is <xsl:value-of select="Personal_Information/Age"/><xsl:text> years old.&#10;</xsl:text>
  </xsl:if>
</xsl:template>

</xsl:stylesheet>
