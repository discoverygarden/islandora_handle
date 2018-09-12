<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
    xmlns="http://www.loc.gov/mods/v3" 
    xmlns:mods="http://www.loc.gov/mods/v3" 
    exclude-result-prefixes="mods"
    version="1.0">
    <xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes" media-type="text/xml"/>
    
    <xsl:param name='handle_value'>this_is_not_a_handle</xsl:param>
    
    <xsl:template match="/">
        <xsl:apply-templates/>
    </xsl:template>

    <xsl:template match="mods:mods">
        <xsl:copy>
            <xsl:apply-templates select="node()|@*"/>
            <identifier type="hdl">
                <xsl:value-of select="$handle_value"/>
            </identifier>
        </xsl:copy>
    </xsl:template>
    
    <!-- delete handle ID at the top... -->
    <xsl:template match="mods:mods/mods:identifier[@type='hdl']"/>
    
    <xsl:template match="node()|@*">
       <xsl:copy>
           <xsl:apply-templates select="node()|@*"/>
       </xsl:copy>
    </xsl:template>
</xsl:stylesheet>
