<?xml version='1.0' encoding="ISO-8859-1"?>
<xsl:stylesheet xmlns:xsl='http://www.w3.org/1999/XSL/Transform' version='1.0'>

  <!-- FileName: namespace46 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 7.1.3 Creating Attributes -->
  <!-- Creator: Philip Strube -->
  <!-- Purpose: Create attribute with NCName and newly-introduced namespace. -->
  <!-- Processor will have to invent a prefix because none has been established for that namespace. -->

<xsl:output method="xml"/>

<xsl:template match="/">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="*">
  <xsl:copy>
    <xsl:attribute name="objectID" namespace="http://www.fiscus.de">
      <xsl:number level="any" count="*"/>
    </xsl:attribute>
    <xsl:apply-templates select="@*|node()|comment()|processing-instruction()"/>
  </xsl:copy>
</xsl:template>

</xsl:stylesheet>
