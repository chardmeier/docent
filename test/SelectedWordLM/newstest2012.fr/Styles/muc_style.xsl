<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
                xmlns:mmax="org.eml.MMAX2.discourse.MMAX2DiscourseLoader"
                xmlns:response="www.eml.org/NameSpaces/response"
		xmlns:case="www.eml.org/NameSpaces/case"
		xmlns:morph="www.eml.org/NameSpaces/morph"
		xmlns:pos="www.eml.org/NameSpaces/pos"
                xmlns:chunk="www.eml.org/NameSpaces/chunk"
		xmlns:enamex="www.eml.org/NameSpaces/enamex"
		xmlns:sentence="www.eml.org/NameSpaces/sentence"
		xmlns:para="www.eml.org/NameSpaces/para"
                xmlns:section="www.eml.org/NameSpaces/section"
                xmlns:coref="www.eml.org/NameSpaces/coref">
 <xsl:output method="text" indent="no" omit-xml-declaration="yes"/>
<xsl:strip-space elements="*"/>


<!--  SECTION: 1 EMPTY LINE SEPARATED -->
<xsl:template match="section:markable" mode="opening">
<xsl:text>
</xsl:text>
</xsl:template>

<!--  SENTENCE: 1 SENTENCE PER LINE -->
<xsl:template match="sentence:markable" mode="closing">
<xsl:text>

</xsl:text>
</xsl:template>

<xsl:template match="words">
<xsl:apply-templates/>
</xsl:template>

<xsl:template match="word">
<xsl:value-of select="mmax:registerDiscourseElement(@id)"/>
<xsl:text> </xsl:text>
<xsl:apply-templates select="mmax:getStartedMarkables(@id)" mode="opening"/>
<xsl:value-of select="mmax:setDiscourseElementStart()"/>
<xsl:apply-templates/>
<xsl:value-of select="mmax:setDiscourseElementEnd()"/>
<xsl:apply-templates select="mmax:getEndedMarkables(@id)" mode="closing"/>
</xsl:template>

<xsl:template match="coref:markable" mode="opening">
<xsl:value-of select="mmax:startBold()"/>
<xsl:value-of select="mmax:addLeftMarkableHandle(@mmax_level, @id,'[')"/>
<xsl:value-of select="mmax:endBold()"/> 
</xsl:template>

<xsl:template match="coref:markable" mode="closing">
<xsl:value-of select="mmax:startBold()"/> 
<xsl:value-of select="mmax:addRightMarkableHandle(@mmax_level, @id,']')"/>
<xsl:value-of select="mmax:endBold()"/> 
</xsl:template>

<xsl:template match="response:markable" mode="opening">
<xsl:value-of select="mmax:startBold()"/>
<xsl:value-of select="mmax:addLeftMarkableHandle(@mmax_level, @id,'*')"/>
<xsl:value-of select="mmax:endBold()"/> 
</xsl:template>

<xsl:template match="response:markable" mode="closing">
<xsl:value-of select="mmax:startBold()"/> 
<xsl:value-of select="mmax:addRightMarkableHandle(@mmax_level, @id,'*')"/>
<xsl:value-of select="mmax:endBold()"/> 
</xsl:template>

<xsl:template match="para:markable" mode="opening">
</xsl:template>

<xsl:template match="para:markable" mode="closing">
<xsl:text>

</xsl:text>
</xsl:template>

<xsl:template match="section:markable" mode="opening">
<xsl:if test="(@name='docid')">
<xsl:value-of select="mmax:startBold()"/>
<xsl:text>DOCID: </xsl:text>
<xsl:value-of select="mmax:endBold()"/>
</xsl:if>

<xsl:if test="(@name='docno')">
<xsl:value-of select="mmax:startBold()"/>
<xsl:text>DOCNO: </xsl:text>
<xsl:value-of select="mmax:endBold()"/>
</xsl:if>

<xsl:if test="(@name='doctype')">
<xsl:value-of select="mmax:startBold()"/>
<xsl:text>DOCTYPE: </xsl:text>
<xsl:value-of select="mmax:endBold()"/>
</xsl:if>

<xsl:if test="(@name='datetime')">
<xsl:value-of select="mmax:startBold()"/>
<xsl:text>DATE_TIME: </xsl:text>
<xsl:value-of select="mmax:endBold()"/>
</xsl:if>

<xsl:if test="(@name='body')">
<xsl:value-of select="mmax:startBold()"/>
<xsl:text>BODY: </xsl:text>
<xsl:value-of select="mmax:endBold()"/>
</xsl:if>

<xsl:if test="(@name='endtime')">
<xsl:value-of select="mmax:startBold()"/>
<xsl:text>END_TIME: </xsl:text>
<xsl:value-of select="mmax:endBold()"/>
</xsl:if>

<xsl:if test="(@name='hl')">
<xsl:value-of select="mmax:startBold()"/>
<xsl:text>HL: </xsl:text>
<xsl:value-of select="mmax:endBold()"/>
</xsl:if>

<xsl:if test="(@name='dd')">
<xsl:value-of select="mmax:startBold()"/>
<xsl:text>DD: </xsl:text>
<xsl:value-of select="mmax:endBold()"/>
</xsl:if>

<xsl:if test="(@name='dateline')">
<xsl:value-of select="mmax:startBold()"/>
<xsl:text>DATELINE: </xsl:text>
<xsl:value-of select="mmax:endBold()"/>
</xsl:if>

<xsl:if test="(@name='storyid')">
<xsl:value-of select="mmax:startBold()"/>
<xsl:text>STORYID: </xsl:text>
<xsl:value-of select="mmax:endBold()"/>
</xsl:if>

<xsl:if test="(@name='slug')">
<xsl:value-of select="mmax:startBold()"/>
<xsl:text>SLUG: </xsl:text>
<xsl:value-of select="mmax:endBold()"/>
</xsl:if>

<xsl:if test="(@name='nwords')">
<xsl:value-of select="mmax:startBold()"/>
<xsl:text>
 NWORDS: </xsl:text>
<xsl:value-of select="mmax:endBold()"/>
</xsl:if>

<xsl:if test="(@name='preamble')">
<xsl:value-of select="mmax:startBold()"/>
<xsl:text>PREAMBLE: </xsl:text>
<xsl:value-of select="mmax:endBold()"/>
</xsl:if>

<xsl:if test="(@name='trailer')">
<xsl:value-of select="mmax:startBold()"/>
<xsl:text>
TRAILER: </xsl:text>
<xsl:value-of select="mmax:endBold()"/>
</xsl:if>
</xsl:template>

<xsl:template match="section:markable" mode="closing">
<xsl:if test="(@name='docid')">
<xsl:text>
</xsl:text>
</xsl:if>
<xsl:if test="(@name='docno')">
<xsl:text>
</xsl:text>
</xsl:if>
<xsl:if test="(@name='doctype')">
<xsl:text>
</xsl:text>
</xsl:if>
<xsl:if test="(@name='datetime')">
<xsl:text>
</xsl:text>
</xsl:if>
<xsl:if test="(@name='body')">
<xsl:text>
</xsl:text>
</xsl:if>
<xsl:if test="(@name='endtime')">
<xsl:text>
</xsl:text>
</xsl:if>
<xsl:if test="(@name='hl')">
<xsl:text>
</xsl:text>
</xsl:if>
<xsl:if test="(@name='dd')">
<xsl:text>
</xsl:text>
</xsl:if>
<xsl:if test="(@name='dateline')">
<xsl:text>
</xsl:text>
</xsl:if>
<xsl:if test="(@name='storyid')">
<xsl:text>
</xsl:text>
</xsl:if>
<xsl:if test="(@name='slug')">
<xsl:text>
</xsl:text>
</xsl:if>
<xsl:if test="(@name='nwords')">
<xsl:text>
</xsl:text>
</xsl:if>
<xsl:if test="(@name='preamble')">
<xsl:text></xsl:text>
</xsl:if>
<xsl:if test="(@name='trailer')">
<xsl:text>
</xsl:text>
</xsl:if>
</xsl:template>

</xsl:stylesheet>
