<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: impincl14 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 5.6 Overriding Template Rules -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Show that apply-imports has limited scope of rules to search. -->
  <!-- "xsl:apply-imports processes the current node using only template rules that
     were imported into the stylesheet CONTAINING THE CURRENT TEMPLATE RULE...."
     The apply-imports will be done in c.xsl; rules from r won't apply at that time. -->

<xsl:import href="r.xsl"/>
<xsl:import href="c.xsl"/>

<xsl:template match="doc">
  <out>
    <xsl:apply-templates select="title"/><!-- c, e, r all have templates for this. -->
  </out>
</xsl:template>

</xsl:stylesheet>