<?xml version="1.0"?> 
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:param name="pParam" select="string('!From p!')"/>

<xsl:template match="publisher">
  p-publisher: 
  <xsl:value-of select="."/>
</xsl:template>
   
</xsl:stylesheet>
