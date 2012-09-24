<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: select54 -->
  <!-- Document: http://www.w3.org/TR/Xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 3.3 Node Sets -->
  <!-- Purpose: NodeSet union using a function that returns a node set (key) and 
       an axis. -->
  <!-- Creator: Carmelo Montanez --><!-- Expression014 in NIST suite -->

<xsl:key name="key1" match="book" use="@author"></xsl:key>

<xsl:template match="doc">
  <out><xsl:text>
</xsl:text>
    <xsl:apply-templates select="child::article|key('key1','Carmelo Montanez')"/>
  </out>
</xsl:template>

<xsl:template match = "*">
  <xsl:value-of select = "."/><xsl:text>
    </xsl:text>
</xsl:template>

</xsl:stylesheet>