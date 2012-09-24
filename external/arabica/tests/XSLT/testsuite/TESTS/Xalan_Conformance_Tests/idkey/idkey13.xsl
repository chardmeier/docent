<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: idkey13 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 12.2 -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test for xsl:key matching multiple nodes on same looked-up value. -->
  <!-- "There can be multiple keys in a document with the same key name, same key value,
       but different nodes." -->

<xsl:key name="mykey" match="div" use="title" />

<xsl:template match="doc">
 <root>
  <xsl:value-of select="count(div)" /> divisions:
  <xsl:value-of select="key('mykey', 'Introduction')/p"/>
  <xsl:value-of select="key('mykey', 'Stylesheet Structure')/p"/>
  The next key finds two divisions:
  <xsl:for-each select="key('mykey', 'Expressions')">
    <xsl:value-of select="p"/>
  </xsl:for-each>
 </root>
</xsl:template>

</xsl:stylesheet>
