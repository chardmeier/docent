<?xml version="1.0"?> 
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: copy22 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Creator: David Marston -->
  <!-- Section: 7.5 -->
  <!-- Purpose: Ensure that external entity reference with high-byte character works in copy. -->

<xsl:output method="xml" encoding="ISO-8859-1"/>
<!-- With this output encoding, should get one byte of xBE for the &frac34 -->

<xsl:template match="@*|node()">
  <xsl:copy>
    <xsl:apply-templates select="@*|node()"/>
  </xsl:copy>
</xsl:template>

</xsl:stylesheet>
