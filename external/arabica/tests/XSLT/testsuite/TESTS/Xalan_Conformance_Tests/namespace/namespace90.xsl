<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: namespace90 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 7.1.2 Creating Elements -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test crossing prefix set at outer level with URI also attached to different prefix in local decl. -->
  <!-- NOTE: Processor developers could legitimately disagree about where the "p2" name
     has to be in scope on the result. It must be in effect for yyy, but could be for foo
     as well. The spec doesn't address this point. (Also, p1 must be reset for yyy.) -->

<xsl:template match = "/">
  <out xmlns:p1="http://xyz">
    <xsl:element name="p1:foo" namespace="http://barz.com" xmlns:p2="http://barz.com">
      <yyy/>
    </xsl:element>
  </out>
</xsl:template>

</xsl:stylesheet>