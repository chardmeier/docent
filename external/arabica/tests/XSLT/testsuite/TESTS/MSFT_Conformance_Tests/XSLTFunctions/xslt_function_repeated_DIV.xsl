<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<!-- Purpose: Test of repeated division. [Bug 71647] -->

<xsl:template match="doc">
  <out>
    <xsl:value-of select="n0 div n1 div n2 div n3"/><xsl:text>,</xsl:text>
    <xsl:value-of select="n0 div n1 div n2 div n3 div n4"/><xsl:text>,</xsl:text>
    <xsl:value-of select="n0 div n1 div n2 div n3 div n4 div n5"/>
  </out>
</xsl:template>
</xsl:stylesheet>
