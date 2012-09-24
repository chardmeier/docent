<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- FileName: idkey49 -->
  <!-- Document: http://www.w3.org/TR/xslt -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 12.4 Generate-ID  -->
  <!-- Creator: David Marston -->
  <!-- Purpose: Test generate-id() when nodes are coming from different documents.
    All IDs should be distinct. -->

<xsl:output method="xml" encoding="UTF-8" indent="no"/>

<xsl:template match="doc">
  <out>
    <!-- Get in position so we have nodes on the following axis. -->
    <xsl:apply-templates select="side"/>
  </out>
</xsl:template>

<xsl:template match="side">
  <!-- Build up a string containing generated IDs for nodes on the following axis, plus attributes they carry. -->
  <xsl:variable name="accumulated">
    <!-- Since call-template doesn't change context, iterate by position number. -->
    <xsl:call-template name="nextnode">
      <xsl:with-param name="this" select="1" />
      <xsl:with-param name="max" select="count(document(../a)//node()|following::node()|following::*/@*)" />
      <xsl:with-param name="idset" select="'+'" />
      <!-- Use + as delimiter to avoid spoofs from adjacent strings.
           Returned string from generate-id() can't contain +. -->
    </xsl:call-template>
  </xsl:variable>
  <!-- Summary data, so we have output when we pass. -->
  <xsl:text>Number of IDs accumulated: </xsl:text>
  <xsl:value-of select="count(document(../a)//node()|following::node()|following::*/@*)"/>
  <!-- Now, take one node of each kind, whose generated ID should not be in the accumulated string,
    surround generated ID by + to avoid substring matches, and see if it's in there. -->
  <!-- See if we duplicated an ID with the root -->
  <xsl:if test="contains($accumulated,concat('+',generate-id(/),'+'))">
    <xsl:text>FAIL on root node whose ID is </xsl:text>
    <xsl:value-of select="generate-id(/)"/>
  </xsl:if>
  <!-- See if we duplicated an ID with the element -->
  <xsl:if test="contains($accumulated,concat('+',generate-id(.),'+'))">
    <xsl:text>FAIL on side node whose ID is </xsl:text>
    <xsl:value-of select="generate-id(.)"/>
  </xsl:if>
  <!-- See if we duplicated an ID with the attribute -->
  <xsl:if test="contains($accumulated,concat('+',generate-id(./@att),'+'))">
    <xsl:text>FAIL on side/@att node whose ID is </xsl:text>
    <xsl:value-of select="generate-id(./@att)"/>
  </xsl:if>
  <!-- See if we duplicated an ID with the text node -->
  <xsl:if test="contains($accumulated,concat('+',generate-id(./text()),'+'))">
    <xsl:text>FAIL on side/text() node whose ID is </xsl:text>
    <xsl:value-of select="generate-id(./text())"/>
  </xsl:if>
  <!-- See if we duplicated an ID with the comment node -->
  <xsl:if test="contains($accumulated,concat('+',generate-id(./comment()),'+'))">
    <xsl:text>FAIL on side/comment() node whose ID is </xsl:text>
    <xsl:value-of select="generate-id(./comment())"/>
  </xsl:if>
  <!-- See if we duplicated an ID with the PI node -->
  <xsl:if test="contains($accumulated,concat('+',generate-id(./processing-instruction('s-pi')),'+'))">
    <xsl:text>FAIL on side/processing-instruction('s-pi') node whose ID is </xsl:text>
    <xsl:value-of select="generate-id(./processing-instruction('s-pi'))"/>
  </xsl:if>
  <!-- See if we duplicated an ID with a namespace node -->
  <xsl:if test="contains($accumulated,concat('+',generate-id(./namespace::*[1]),'+'))">
    <xsl:text>FAIL on side/namespace::*[1] node whose ID is </xsl:text>
    <xsl:value-of select="generate-id(./namespace::*[1])"/>
  </xsl:if>
  <!-- See if we duplicated an ID with an element from another document -->
  <xsl:if test="contains($accumulated,concat('+',generate-id(document('idkey49a.xml')//body),'+'))">
    <xsl:text>FAIL on 49a body node whose ID is </xsl:text>
    <xsl:value-of select="generate-id(document('idkey49a.xml')//body)"/>
  </xsl:if>
  <!-- See if we duplicated an ID with an attribute from another document -->
  <xsl:if test="contains($accumulated,concat('+',generate-id(document('idkey49a.xml')//body/@att),'+'))">
    <xsl:text>FAIL on 49a body node whose ID is </xsl:text>
    <xsl:value-of select="generate-id(document('idkey49a.xml')//body/@att)"/>
  </xsl:if>
  <!-- See if we duplicated an ID with a text node from another document -->
  <xsl:if test="contains($accumulated,concat('+',generate-id(document('idkey49a.xml')//body/text()),'+'))">
    <xsl:text>FAIL on 49a body node whose ID is </xsl:text>
    <xsl:value-of select="generate-id(document('idkey49a.xml')//body/text())"/>
  </xsl:if>
  <!-- See if we duplicated an ID with a comment node from another document -->
  <xsl:if test="contains($accumulated,concat('+',generate-id(document('idkey49a.xml')//comment()),'+'))">
    <xsl:text>FAIL on 49a body node whose ID is </xsl:text>
    <xsl:value-of select="generate-id(document('idkey49a.xml')//comment())"/>
  </xsl:if>
</xsl:template>

<xsl:template name="nextnode">
  <xsl:param name="this"/>
  <xsl:param name="max"/>
  <xsl:param name="idset"/>
  <!-- Params this and max are index numbers, idset is the string we're accumulating. -->
  <xsl:variable name="this-id" select="generate-id((document(../a)//node()|following::node()|following::*/@*)[position() = $this])"/>
  <xsl:choose>
    <xsl:when test="$this &lt;= $max">
      <!-- Recurse, adding current ID to string of all IDs, with separators -->
      <xsl:call-template name="nextnode">
        <xsl:with-param name="this" select="$this+1" />
        <xsl:with-param name="max" select="$max" />
        <xsl:with-param name="idset" select="concat($idset,$this-id,'+')" />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <!-- "return" the final idset -->
      <xsl:value-of select="$idset"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>