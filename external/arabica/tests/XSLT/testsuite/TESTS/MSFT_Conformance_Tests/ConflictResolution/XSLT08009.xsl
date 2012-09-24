��<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"><xsl:output method="xml" omit-xml-declaration="yes" indent="yes"/>


  <xsl:template match="bookstore/book">
    
          
          <xsl:apply-templates select="author"/>
	
           <xsl:apply-templates select="author/last-name"/>

	
          

		

 </xsl:template>

 <xsl:template match="author" priority="2">
- <SPAN style="color=green"> <xsl:value-of select="my:country" /> </SPAN>
  </xsl:template>
 <xsl:template match="last-name" priority="1.22">
- <SPAN style="color=green">  <xsl:value-of select="." />   </SPAN>
  </xsl:template>
 <xsl:template match="last-name" priority="]N">
- <SPAN style="color=red" ID="test">
  <xsl:value-of select="." /> 
  </SPAN>
  </xsl:template>
 

 <xsl:template match="author" priority="22">
- <SPAN style="color=red" ID="test">
  <xsl:value-of select="my:country" /> 
  </SPAN>
  </xsl:template>

 <xsl:template match="*" priority="-5">
	<xsl:apply-templates select="*"/>
 </xsl:template>



</xsl:stylesheet>
