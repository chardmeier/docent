<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: AXES114 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 3.3 -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test for preceding::foo[1] vs. (preceding::foo)[1], as discussed in
     the first NOTE in section 3.3 -->

<xsl:template match="/">
  <out>
    <xsl:for-each select="//baz">
      <xsl:value-of select="preceding::foo[1]/@att1"/><xsl:text>, </xsl:text>
      <xsl:value-of select="(preceding::foo)[1]/@att1"/>
    </xsl:for-each>
  </out>
</xsl:template>

</xsl:stylesheet>
