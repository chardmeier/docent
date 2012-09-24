<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: MATCH24 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 5.2 -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test of three predicates, two being positional. Use last() for one.
     First predicate reduces the set to {a,c,e,g,i,k}.
     Second predicate, taken alone, reduces the set to {f,g,h,i,j,k,l}. -->

<xsl:template match="/">
  <out>
    <xsl:apply-templates select="root/x"/>
  </out>
</xsl:template>

<xsl:template match="x[(position() mod 2)=1][@num &gt; 5][last()]">
  <xsl:value-of select="@spot"/>
  <xsl:text> </xsl:text>
</xsl:template>

</xsl:stylesheet>
