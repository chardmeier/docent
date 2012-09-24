<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: lre06 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 7.1.1 Literal Result Elements-->
  <!-- Creator: Paul Dick -->
  <!-- Purpose: Tests that the value of an attribute of a LRE is 
  	   interpreted as a attribute value template, which can contain
  	   expressions within curly braces({}).	-->

<xsl:template match="docs">
  <out a="{doc1/a}" b="{doc2/doc3/bat}"
       c="{doc2/doc3/a/@level}"
       d="{.//*[starts-with(name(.),'ba')]}"
       e="{.//cat}" f="{'All Done'}">
  </out>
</xsl:template>

</xsl:stylesheet>
