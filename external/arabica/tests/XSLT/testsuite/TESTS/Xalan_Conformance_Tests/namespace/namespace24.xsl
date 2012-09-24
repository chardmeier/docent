<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
      xmlns:axsl="http://www.w3.org/1999/XSL/TransAlias">

<xsl:output method="xml" indent="no"/>

  <!-- FileName: nspc24 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991118 -->
  <!-- Section: 7.1.1 Literal Result Elements -->
  <!-- Creator: Paul Dick -->
  <!-- Purpose: Test basic functionality of namespace-alias. Where XSL elements
       are created by using xsl:element command. -->

<xsl:namespace-alias stylesheet-prefix="axsl" result-prefix="xsl"/>

<xsl:template match="/">
  <xsl:element name="axsl:stylesheet">
  	<xsl:attribute name="version">1.0</xsl:attribute>
    <xsl:apply-templates/>
  </xsl:element>
</xsl:template>

<xsl:template match="block">
  <axsl:template match="{.}">
     <axsl:apply-templates/>
  </axsl:template>
</xsl:template>

</xsl:stylesheet>
