<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: idkey18 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 12.2 -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test combination of key() and document() as suggested in spec. -->

<xsl:key name="bib" match="entry" use="@name" />

<xsl:template match="doc">
 <root>
  <xsl:apply-templates/>
 </root>
</xsl:template>

<xsl:template match="bibref">
  <xsl:variable name="lookup" select="."/>
  <xsl:for-each select="document('bib.xml')">
    <xsl:apply-templates select="key('bib',$lookup)"/>
  </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
