<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:template match="/">
  <out>
    <xsl:value-of select="format-number(111222333.445,'##,##,###.00')"/>
  </out>
</xsl:template>

</xsl:stylesheet>
