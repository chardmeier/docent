<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- CaseName: numbering11 -->
  <!-- Author: David Marston -->
  <!-- Purpose: Test of grouping attributes. -->
  <!-- SpecCitation: Rec="XSLT" Version="1.0" type="OASISptr1" place="id(number)/ulist[2]/item[3]/p[1]/text()[6]" -->
  <!-- SpecCitation: Rec="XSLT" Version="1.0" type="OASISptr1" place="id(number)/ulist[1]/item[3]/p[1]/text()[1]" -->
  <!-- SpecCitation: Rec="XSLT" Version="1.0" type="OASISptr1" place="id(convert)/p[2]/text()[5]" -->
  <!-- SpecCitation: Rec="XSLT" Version="1.0" type="OASISptr1" place="id(convert)/ulist[1]/item[1]/p[1]/text()[1]" -->
  <!-- SpecCitation: Rec="XSLT" Version="1.0" type="OASISptr1" place="id(convert)/p[6]/text()[1]" -->
  <!-- SpecCitation: Rec="XSLT" Version="1.0" type="OASISptr1" place="id(convert)/p[6]/text()[2]" -->
  <!-- Scenario: operation="standard-XML" -->

<xsl:template match="doc">
  <out>
    <xsl:apply-templates/>
  </out>
</xsl:template>

<xsl:template match="note">
  <xsl:number level="any" from="chapter" format="(1) " grouping-size="2" grouping-separator="," />
  <xsl:apply-templates/>
</xsl:template>

</xsl:stylesheet>
