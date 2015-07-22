<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
    xmlns="http://purl.org/dc/elements/1.1/" 
    xmlns:dc="http://purl.org/dc/elements/1.1/" 
    xmlns:oai_dc="http://www.openarchives.org/OAI/2.0/oai_dc/" 
    exclude-result-prefixes="oai_dc dc"
    version="1.0">
    <xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes" media-type="text/xml"/>
    
    <xsl:param name='handle_value'>this_is_not_a_handle</xsl:param>
    
    <xsl:template match="/">
        <xsl:apply-templates/>
    </xsl:template>
    
    <xsl:template match="dc:dc">
        <xsl:copy>
            <xsl:apply-templates/>
            <identifier type="hdl">
                <xsl:value-of select="$handle_value"/>
            </identifier>
        </xsl:copy>
    </xsl:template>
    
    <!-- delete handle ID at the top... -->
    <xsl:template match="oai_dc:dc/dc:identifier[@type='hdl']"/>
    
    <xsl:template match="node()|@*">
       <xsl:copy>
           <xsl:apply-templates select="node()|@*"/>
       </xsl:copy>
    </xsl:template>
</xsl:stylesheet>
