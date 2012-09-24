<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method="html"/>

  <!-- FileName: attribset17 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 7.1.3 Creating Attributes -->
  <!-- Creator: Paul Dick -->
  <!-- Purpose: Verify that 'checked' attribute of HTML element input is correctly set. -->

<xsl:template match="ticket/input">
<HTML>
  <Form>
	<xsl:value-of select="zoneone"/> 
	<Input Type="checkbox">
	  <xsl:if test="zoneone/@checked='1'">
	    <xsl:attribute name="CHECKED">CHECKED</xsl:attribute>
      </xsl:if>
    </Input>
  </Form>
</HTML>
</xsl:template>

</xsl:stylesheet>