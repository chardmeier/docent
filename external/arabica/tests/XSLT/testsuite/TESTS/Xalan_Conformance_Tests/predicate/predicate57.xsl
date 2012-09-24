<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

  <!-- FileName: predicate57 -->
  <!-- Document: http://www.w3.org/TR/xpath -->
  <!-- DocVersion: 19991116 -->
  <!-- Section: 2.4 -->
  <!-- Creator: David Marston, based on an idea by Mukund Raghavachari -->
  <!-- Purpose: Test use of count() in a predicate to count children. -->

<xsl:output method="xml" encoding="UTF-8"/>

<xsl:template match="/">
  <out>
    <xsl:text>tr nodes: </xsl:text><xsl:value-of select="count(//tr)"/>
    <xsl:text>, tr nodes with 3 td children: </xsl:text>
    <xsl:value-of select="count(//tr[count(./td) = 3])"/><xsl:text>
</xsl:text>
    <nodes>
      <xsl:for-each select="//tr[count(./td) = 3]">
        <xsl:for-each select="td">
          <xsl:value-of select="."/>
          <xsl:if test="following-sibling::td">
            <xsl:text>, </xsl:text>
          </xsl:if>
        </xsl:for-each><xsl:text>
  </xsl:text>
      </xsl:for-each>
    </nodes>
  </out>
</xsl:template>

</xsl:stylesheet>
