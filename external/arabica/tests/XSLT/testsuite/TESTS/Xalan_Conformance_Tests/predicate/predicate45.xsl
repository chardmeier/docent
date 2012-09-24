<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: predicate45 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 2.4 -->
  <!-- Creator: Carmelo Montanez --><!-- LocationPath007 in NIST suite -->
  <!-- Purpose: Test a predicate that uses elaborate complex expressions -->

<xsl:template match="doc">
  <out>
    <xsl:value-of select = "element1[(((((2*10)-4)+9) div 5) mod 3 )]"/>
  </out>
</xsl:template>

</xsl:stylesheet>
