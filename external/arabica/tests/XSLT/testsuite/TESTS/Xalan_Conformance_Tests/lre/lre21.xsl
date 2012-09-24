<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
	xmlns:ped="http://tester.com"
	xmlns:ext="http://somebody.elses.extension">

  <!-- FileName: lre21 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 7.1.1 Literal Result Elements-->
  <!-- Creator: David Marston -->
  <!-- Purpose: Exclude namespace node for an extension namespace via local declaration. -->
  <!-- Elaboration: The created element node will also have a copy of the namespace nodes 
       that were present on the element node in the stylesheet tree with the exception 
       of any namespace node whose string-value is the XSLT namespace URI, a namespace 
       URI declared as an extension namespace, or a namespace URI designated as an 
       excluded namespace. -->

<xsl:template match="doc">
  <out english="to leave" xsl:extension-element-prefixes="ext"/>
</xsl:template>

</xsl:stylesheet>