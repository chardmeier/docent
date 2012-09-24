<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: namespace68 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 7.1.2 Creating Elements -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Set default namespace in outer, then specify namespace for inner; add prefixed decl. -->
  <!-- NOTE: Processor developers could legitimately disagree about where the "p2" name
     has to be in scope on the result, if anywhere. Since there are no LREs inside foo,
     there is no requirement to pick up and emit the p2 declaration.
     The spec doesn't address this point. -->

<xsl:template match = "/">
  <out xmlns="http://abc">
    <xsl:element namespace="http://abc" name="foo" xmlns:p2="http://barz.com">
      <xsl:element name="yyy"/>
    </xsl:element>
  </out>
</xsl:template>

</xsl:stylesheet>