<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- CaseName: numbering54 -->
  <!-- Author: David Marston -->
  <!-- Purpose: Test xsl:number with from and count defaulted, level=multiple. -->
  <!-- SpecCitation: Rec="XSLT" Version="1.0" type="OASISptr1" place="id(number)/ulist[2]/item[2]/p[1]/text()[1]" -->
  <!-- SpecCitation: Rec="XSLT" Version="1.0" type="OASISptr1" place="id(number)/ulist[1]/item[2]/p[1]/text()[3]" -->
  <!-- SpecCitation: Rec="XSLT" Version="1.0" type="OASISptr1" place="id(convert)/ulist[1]/item[1]/p[1]/text()[1]" -->
  <!-- Scenario: operation="standard-XML" -->

<xsl:template match="doc">
  <out><xsl:apply-templates/></out>
</xsl:template>

<xsl:template match="note">
  <xsl:number level="multiple" format="1"/><xsl:text>: </xsl:text><xsl:value-of select="."/><xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="text()"><!-- To suppress empty lines --><xsl:apply-templates/></xsl:template>

</xsl:stylesheet>
