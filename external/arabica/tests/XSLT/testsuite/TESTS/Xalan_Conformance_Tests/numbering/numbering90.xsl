<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- CaseName: numbering90 -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test counting only some nodes, with key() in count pattern. -->
  <!-- SpecCitation: Rec="XSLT" Version="1.0" type="OASISptr1" place="id(number)/ulist[2]/item[1]/p[1]/text()[5]" -->
  <!-- SpecCitation: Rec="XSLT" Version="1.0" type="OASISptr1" place="id(number)/ulist[1]/item[2]/p[1]/text()[1]" -->
  <!-- SpecCitation: Rec="XSLT" Version="1.0" type="OASISptr1" place="id(number)/ulist[1]/item[3]/p[1]/text()[1]" -->
  <!-- SpecCitation: Rec="XSLT" Version="1.0" type="OASISptr1" place="id(convert)/p[2]/text()[5]" -->
  <!-- SpecCitation: Rec="XSLT" Version="1.0" type="OASISptr1" place="id(convert)/ulist[1]/item[1]/p[1]/text()[1]" -->
  <!-- Scenario: operation="standard-XML" -->

<xsl:key name="key_of_f" match="note" use="@flag"/>

<xsl:template match="doc">
  <out>
    <xsl:apply-templates/>
  </out>
</xsl:template>

<xsl:template match="note">
  <xsl:number level="single" from="doc|chapter" count="key('key_of_f','yes')" format="(1) "/>
  <xsl:apply-templates/>
</xsl:template>

</xsl:stylesheet>