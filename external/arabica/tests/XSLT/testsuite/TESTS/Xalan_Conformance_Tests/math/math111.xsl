<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: MATH111 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 4.4 -->
  <!-- Creator: Gordon Chiu -->
  <!-- Purpose: Test of string to number conversion for small (yet still representable) decimal numbers. -->
	<xsl:template match="doc">
		<out>
			<xsl:apply-templates/>
		</out>
	</xsl:template>
	<xsl:template match="number">
		<pos><xsl:value-of select="string(number(.))"/></pos>
		<neg><xsl:value-of select="string(-1 * number(.))"/></neg>
	</xsl:template>
</xsl:stylesheet>
