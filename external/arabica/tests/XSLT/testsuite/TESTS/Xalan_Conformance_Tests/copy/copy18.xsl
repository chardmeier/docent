<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<!-- FileName: copy18 -->
<!-- Document: http://www.w3.org/TR/xslt -->
<!-- DocVersion: 1999116 -->
<!-- Section: 11.3 -->
<!-- Creator: Gertjan van Son -->
<!-- Purpose: Test for copy-of with union of attribute nodes. -->

<xsl:template match="TEST">
   <xsl:element name="out">
      <xsl:apply-templates/>
   </xsl:element>
</xsl:template>

<xsl:template match="ELEMENT">
   <xsl:element name="item">
      <xsl:copy-of select="@x|@z"/>
   </xsl:element>
</xsl:template>

</xsl:stylesheet>