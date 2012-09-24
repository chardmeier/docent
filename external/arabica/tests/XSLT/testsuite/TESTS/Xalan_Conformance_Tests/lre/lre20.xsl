<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
	xmlns:ped="http://tester.com"
	xmlns:ext="http://somebody.elses.extension"
      extension-element-prefixes="ext">

  <!-- FileName: lre20 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 7.1.1 Literal Result Elements-->
  <!-- Creator: Paul Dick -->
  <!-- Purpose: Show that the namespace node for an extension namespace is automatically excluded. -->
  <!-- Elaboration: The created element node will also have a copy of the namespace nodes 
       that were present on the element node in the stylesheet tree with the exception 
       of any namespace node whose string-value is the XSLT namespace URI, a namespace 
       URI declared as an extension namespace, or a namespace URI designated as an 
       excluded namespace. -->

<xsl:template match="doc">
  <out english="to leave"/>
</xsl:template>

</xsl:stylesheet>