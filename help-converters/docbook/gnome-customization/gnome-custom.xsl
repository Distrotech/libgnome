<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'
                xmlns="http://www.w3.org/TR/xhtml1/transitional"
                exclude-result-prefixes="#default">

<!--****************************Parameters**********************-->
<xsl:param name="local.l10n.xml" select="document('l10n.xml')"/>

<!--******************************Admonitions*******************-->

<!--********************************Variables*******************-->

<!-- Should graphics be included in admonitions? 0 or 1 -->
<xsl:param name="admon.graphics" select="1"/>

<!-- Specifies the default path for admonition graphics -->
<xsl:param name="admon.graphics.path">./stylesheet/</xsl:param>

<!-- Specifies the default graphic file if none is given. -->
<xsl:param name="graphic.default.extension" select="'.png'" doc:type="string"/>

<!--*******************************Templates********************-->

<!-- Moves the title just to the right of the admonition graphic. -->
<xsl:template name="graphical.admonition">
  <div class="{name(.)}">
  <xsl:if test="$admon.style != ''">
    <xsl:attribute name="style">
      <xsl:value-of select="$admon.style"/>
    </xsl:attribute>
  </xsl:if>
  <table border="0">
    <tr>
      <td rowspan="2" align="center" valign="top">
        <xsl:attribute name="width">
          <xsl:call-template name="admon.graphic.width"/>
        </xsl:attribute>
        <img>
          <xsl:attribute name="src">
            <xsl:call-template name="admon.graphic"/>
          </xsl:attribute>
        </img>
      </td>
      <th align="left" valign="top">
        <xsl:call-template name="anchor"/>
        <xsl:apply-templates select="." mode="object.title.markup"/>
      </th>
    </tr>
    <tr>
      <td colspan="2" align="left" valign="top">
        <xsl:apply-templates/>
      </td>
    </tr>
  </table>
  </div>
</xsl:template>


<!-- ***************************Author**********************-->

<!-- Adds the string "Written By:" to the author tag in the 
	 title page.  Also adds a maintainer by the role attribute 
	 to the author tag so the program maintainer can be specified 
	 and the application name in the condition attribute. -->

<xsl:template match="author" mode="titlepage.mode">

  <xsl:call-template name="person.name"/>
  <br/>

</xsl:template>

<xsl:template match="authorgroup" mode="titlepage.mode">

  <xsl:choose>
    <xsl:when test="@role=''">
      <h2>
	  <xsl:call-template name="gentext" mode="titlepage.mode">
	    <xsl:with-param name="key" select="'Author'"/>
	  </xsl:call-template>
	  </h2>
	  <xsl:apply-templates mode="titlepage.mode"/>
    </xsl:when>
    <xsl:when test="@role='maintainer'">
    </xsl:when>
  </xsl:choose>
  
</xsl:template>

<!-- **************************Copyright*************************-->

<!-- Makes the copyright years appear as a range. -->
<xsl:param name="make.year.ranges" select="1" doc:type="boolean"/>

<xsl:template match="copyright[1]" mode="titlepage.mode">

  <div class="{name(.)}">
    <h2>
	  <xsl:call-template name="gentext">
	    <xsl:with-param name="key" select="'Copyright'"/>
	  </xsl:call-template>
	</h2>
	
    <p class="{name(.)}">
      <xsl:call-template name="gentext">
	    <xsl:with-param name="key" select="'Copyright'"/>
	  </xsl:call-template>
	  <xsl:call-template name="gentext.space"/>
	  <xsl:call-template name="dingbat">
	    <xsl:with-param name="dingbat">copyright</xsl:with-param>
	  </xsl:call-template>
	  <xsl:call-template name="gentext.space"/>
	  <xsl:call-template name="copyright.years">
	    <xsl:with-param name="years" select="year"/>
		<xsl:with-param name="print.ranges" select="$make.year.ranges"/>
		<xsl:with-param name="single.year.ranges"
						select="$make.single.year.ranges"/>
	  </xsl:call-template>
	  <xsl:call-template name="gentext.space"/>
	  <xsl:apply-templates select="holder" mode="titlepage.mode"/>
	</p>
  </div>

</xsl:template>

<!-- ****************************Emphasis************************-->

<!-- Custom template to make the emphasis tag bold the text instead of 
	 the text be italicized. -->
<xsl:template match="emphasis">
  <xsl:call-template name="inline.boldseq"/>
</xsl:template>

<!-- ************************Glossary***************************-->

<!-- Importing the Norman Walsh's stylesheet as the basis. -->

<xsl:template match="glossterm">
  <dt>
    <b>
      <xsl:apply-templates/>
    </b>
  </dt>
</xsl:template>

<xsl:template match="glossentry/glossterm[1]" priority="2">
  <dt>
	<b>
      <xsl:call-template name="anchor">
        <xsl:with-param name="node" select=".."/>
      </xsl:call-template>
      <xsl:apply-templates/>
	</b>
  </dt>
</xsl:template>

<xsl:template match="glossentry/glossdef[@subject]">

	<xsl:if test="position()=2">
	<xsl:text disable-output-escaping="yes">&lt;dd&gt;</xsl:text>
	<xsl:text disable-output-escaping="yes">&lt;div class="informaltable"&gt;</xsl:text>
	<xsl:text disable-output-escaping="yes">&lt;table border="0"&gt;</xsl:text>
	<xsl:text disable-output-escaping="yes">&lt;tbody&gt;</xsl:text>
	</xsl:if>

	<tr>
	<td valign="top">
	<p>
	<xsl:value-of select="@subject"/>
	</p>
	</td>
	<td>
    <xsl:apply-templates select="*[local-name(.) != 'glossseealso']"/>
	</td>
	</tr>

    <xsl:if test="glossseealso">
      <p>
        <xsl:call-template name="gentext.template">
          <xsl:with-param name="context" select="'glossary'"/>
          <xsl:with-param name="name" select="'seealso'"/>
        </xsl:call-template>
        <xsl:apply-templates select="glossseealso"/>
      </p>
    </xsl:if>

	<xsl:if test="position()=last()">
	<xsl:text disable-output-escaping="yes">&lt;/tbody&gt;</xsl:text>
	<xsl:text disable-output-escaping="yes">&lt;/table&gt;</xsl:text>
	<xsl:text disable-output-escaping="yes">&lt;/div&gt;</xsl:text>
	<xsl:text disable-output-escaping="yes">&lt;/dd&gt;</xsl:text>
	</xsl:if>

</xsl:template>

<!-- ****************************Emphasis************************-->

<!-- Custom template to make the guimenu, guisubmenu, guimenuitem and guilabel
	 tags bold the text. -->

	 <xsl:template match="guimenu|guisubmenu|guimenuitem|guilabel">
	 <xsl:call-template name="inline.boldseq"/> </xsl:template>

<!-- *************************Keycombo***************************-->

<xsl:template match="keycombo">
  <xsl:variable name="action" select="@action"/>
  <xsl:variable name="joinchar">
    <xsl:choose>
      <xsl:when test="$action='seq'"><xsl:text> </xsl:text></xsl:when>
      <xsl:when test="$action='simul'">+</xsl:when>
      <xsl:when test="$action='press'">-</xsl:when>
      <xsl:when test="$action='click'">-</xsl:when>
      <xsl:when test="$action='double-click'">-</xsl:when>
      <xsl:when test="$action='other'"></xsl:when>
      <xsl:otherwise>+</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:for-each select="./*">
    <xsl:if test="position()>1"><xsl:value-of select="$joinchar"/></xsl:if>
    <xsl:apply-templates/>
  </xsl:for-each>
</xsl:template>

<!-- *****************************Legalnotice*******************-->

<!-- Puts a link on the title page to each legalnotice. -->
<xsl:param name="generate.legalnotice.link" select="0"/>

<xsl:template match="legalnotice[1]" mode="titlepage.mode">
  <xsl:text>&#xA;</xsl:text>
  <h2>
  <xsl:call-template name="gentext">
    <xsl:with-param name="key" select="'legalnotice'"/>
  </xsl:call-template>
  </h2>
  <xsl:apply-templates mode="titlepage.mode"/>
</xsl:template>

<!-- Makes the address inside the legalnotice inline instead of separated 
  	 from the body of the text.-->
<xsl:template match="legalnotice//address">
  <xsl:param name="suppress-numbers" select="'0'"/>
  <xsl:variable name="vendor" select="system-property('xsl:vendor')"/>

  <xsl:variable name="rtf">
    <xsl:apply-templates mode="inline-address"/>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$suppress-numbers = '0'
                    and @linenumbering = 'numbered'
                    and $use.extensions != '0'
                    and $linenumbering.extension != '0'">
          <xsl:call-template name="number.rtf.lines">
            <xsl:with-param name="rtf" select="$rtf"/>
          </xsl:call-template>
    </xsl:when>

    <xsl:otherwise>
          <xsl:apply-templates mode="inline-address"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- Added a comma after the street, city and postal code. -->
<xsl:template match="street|city|postcode" mode="inline-address">
  <xsl:apply-templates mode="inline-address"/>
  <xsl:text>, </xsl:text>
</xsl:template>

<xsl:template match="state|country" mode="inline-address">   
  <xsl:apply-templates mode="inline-address"/>
</xsl:template>

<!-- *******************************Procedure*******************-->

<xsl:param name="formal.procedures">
  <xsl:if test="procedure[@role='informal']">
    <xsl:text>0</xsl:text>
  </xsl:if>
</xsl:param>

<!-- ****************************Publisher********************-->

<xsl:template match="publisher[1]" mode="titlepage.mode">

  <div class="{name(.)}">
    <h2>
	  <xsl:call-template name="gentext">
	    <xsl:with-param name="key" select="'Publisher'"/>
	  </xsl:call-template>
	</h2>
	
	<xsl:apply-templates mode="titlepage.mode"/>
  </div>
  
</xsl:template>

<!-- ***************************Releaseinfo*********************-->

<!-- Custom template to make the releaseinfo tag italicized. -->
<xsl:template match="releaseinfo" mode="titlepage.mode">
  <p class="{name(.)}"><i>
  <xsl:apply-templates mode="titlepage.mode"/>
  </i></p>
</xsl:template>

<!-- ****************************Revhistory********************-->

<xsl:template match="revhistory" mode="titlepage.mode">
  <xsl:variable name="numcols">
<!--
    <xsl:choose>
      <xsl:when test="//authorinitials">3</xsl:when>
      <xsl:otherwise>2</xsl:otherwise>
    </xsl:choose>
-->
    4
  </xsl:variable>

  <div class="{name(.)}">
    <h2><xsl:call-template name="gentext">
              <xsl:with-param name="key" select="'RevHistory'"/>
            </xsl:call-template>
    </h2>
    <table border="1" width="100%" summary="Revision history">
      <tr>
        <th align="left" valign="top" colspan="1">
          <b>
            <xsl:call-template name="gentext">
              <xsl:with-param name="key" select="'Title'"/>
            </xsl:call-template>
          </b>
        </th>
	<th align="left" valign="top" colspan="1">
          <b>
            <xsl:call-template name="gentext">
              <xsl:with-param name="key" select="'Date'"/>
            </xsl:call-template>
          </b>
        </th>
	<th align="left" valign="top" colspan="1">
          <b>
            <xsl:call-template name="gentext">
              <xsl:with-param name="key" select="'Author'"/>
            </xsl:call-template>
          </b>
        </th>
	<th align="left" valign="top" colspan="1">
          <b>
            <xsl:call-template name="gentext">
              <xsl:with-param name="key" select="'Publisher'"/>
            </xsl:call-template>
          </b>
        </th>
      </tr>
      <xsl:apply-templates mode="titlepage.mode">
        <xsl:with-param name="numcols" select="$numcols"/>
      </xsl:apply-templates>
    </table>
  </div>
</xsl:template>

<xsl:template match="revhistory/revision" mode="titlepage.mode">
  <xsl:param name="numcols" select="'4'"/>  
  <xsl:variable name="revnumber" select=".//revnumber"/>
  <xsl:variable name="revdate"   select=".//date"/>
  <xsl:variable name="revauthor" select=".//authorinitials"/>
  <xsl:variable name="revremark" select=".//revremark|.//revdescription"/>

  <xsl:variable name="author.name">
    <xsl:if test="$revremark">
	  <xsl:value-of select=".//revdescription/para/author"/>
	  			   
				   <!-- select="substring-after( substring-before( $revremark[1], 'Publisher:'),
				   'Author:')"-->
	</xsl:if>
  </xsl:variable>

  <xsl:variable name="person.name">
    <xsl:call-template name="person.name"/>
  </xsl:variable>

  <xsl:variable name="publisher.name">
    <xsl:value-of select="substring-after( $revremark[1], 'Publisher:')"/>
  </xsl:variable>

  <tr>
    <td align="left">
      <p>  
	    <xsl:value-of select="//title"/>
	    <xsl:if test="$revnumber != '1.0'">
	      <xsl:text> V</xsl:text>
	      <xsl:apply-templates select="$revnumber[1]" mode="titlepage.mode"/>
	    </xsl:if>
	  </p>
	</td>

	<td align="left">
	  <p>
		<xsl:value-of select="$revdate"/>
	  </p>
	</td>

	<td align="left">
	  <p>
	    <xsl:value-of select="$author.name"/>
	  </p><p>
	    <xsl:text>(</xsl:text>
		<xsl:value-of select="$revremark[1]/para/email"/>
		<xsl:text>)</xsl:text>
	  </p>
	</td>

	<td align="left">
	  <p>  
		<xsl:value-of select="$publisher.name"/>
	  </p>
	</td>
  </tr>
</xsl:template>

<xsl:template match="revision/revnumber" mode="titlepage.mode">
  <xsl:apply-templates mode="titlepage.mode"/>
</xsl:template>

<xsl:template match="revision/date" mode="titlepage.mode">
  <xsl:apply-templates mode="titlepage.mode"/>
</xsl:template>

<xsl:template match="revision/authorinitials" mode="titlepage.mode">
  <xsl:apply-templates mode="titlepage.mode"/>
</xsl:template>

<xsl:template match="revision/revremark" mode="titlepage.mode">
  <xsl:apply-templates mode="titlepage.mode"/>
</xsl:template>

<xsl:template match="revision/revdescription" mode="titlepage.mode">
  <xsl:apply-templates mode="titlepage.mode"/>
</xsl:template>



<!-- ==================================================================== -->

<!-- This stylesheet was created by template/titlepage.xsl; do not edit it by hand. -->


  

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="book.titlepage.recto">
    
  <xsl:choose xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:when xmlns:xsl="http://www.w3.org/1999/XSL/Transform" test="bookinfo/title">
      <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="book.titlepage.recto.auto.mode" select="bookinfo/title"/>
    </xsl:when>
    <xsl:when xmlns:xsl="http://www.w3.org/1999/XSL/Transform" test="title">
      <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="book.titlepage.recto.auto.mode" select="title"/>
    </xsl:when>
  </xsl:choose>

    
  <xsl:choose xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:when xmlns:xsl="http://www.w3.org/1999/XSL/Transform" test="bookinfo/subtitle">
      <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="book.titlepage.recto.auto.mode" select="bookinfo/subtitle"/>
    </xsl:when>
    <xsl:when xmlns:xsl="http://www.w3.org/1999/XSL/Transform" test="subtitle">
      <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="book.titlepage.recto.auto.mode" select="subtitle"/>
    </xsl:when>
  </xsl:choose>

    
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="book.titlepage.recto.auto.mode" select="bookinfo/authorgroup"/>
    
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="book.titlepage.recto.auto.mode" select="bookinfo/author"/>
    
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="book.titlepage.recto.auto.mode" select="bookinfo/publisher"/>
    
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="book.titlepage.recto.auto.mode" select="bookinfo/copyright"/>
    
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="book.titlepage.recto.auto.mode" select="bookinfo/legalnotice"/>
    
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="book.titlepage.recto.auto.mode" select="bookinfo/revhistory"/>
    
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="book.titlepage.recto.auto.mode" select="bookinfo/revision"/>
    
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="book.titlepage.recto.auto.mode" select="bookinfo/releaseinfo"/>

</xsl:template>
    
    
    
    
    
    
    
    
    
    
    
    
    
  

  

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="book.titlepage.verso">
  
</xsl:template>
  

  

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="book.titlepage.separator">
  
</xsl:template>

  

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="book.titlepage.before.recto">
    <img align="right" src="stylesheet/gnome-logo-large.png" alt="GNOME Logo"/>
  
</xsl:template>

  

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="book.titlepage.before.verso">
  
</xsl:template>


<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="book.titlepage">
<!-- Added to create a separate titlepage -->
<!--
  <xsl:param name="next" select="."/>
  <xsl:variable name="id">titlepage</xsl:variable>
  <xsl:choose>
    <xsl:when test="$generate.titlepage.link != 0">

      <xsl:variable name="filename">
        <xsl:call-template name="make-relative-filename">
          <xsl:with-param name="base.dir" select="$base.dir"/>
          <xsl:with-param name="base.name" select="concat($id,$html.ext)"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="title">
        <xsl:apply-templates select="." mode="title.markup"/>
      </xsl:variable>

      <xsl:call-template name="write.chunk">
        <xsl:with-param name="filename" select="$filename"/>
        <xsl:with-param name="content">
          <html>
            <head>
              <title><xsl:value-of select="$title"/></title>
            </head>
            <body>
              <xsl:call-template name="body.attributes"/>
	    	  <xsl:call-template name="user.header.navigation"/>

	    	  <xsl:call-template name="header.navigation">
			<xsl:with-param name="next" select="$next"/>
		      </xsl:call-template>

		      <xsl:call-template name="user.header.content"/>
			  <div class="titlepage">
			    <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="book.titlepage.before.recto"/>
			    <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="book.titlepage.recto"/>
			    <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="book.titlepage.before.verso"/>
			    <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="book.titlepage.verso"/>
			    <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="book.titlepage.separator"/>
			  </div>	
		      <xsl:call-template name="user.footer.content"/>

		      <xsl:call-template name="footer.navigation">
			<xsl:with-param name="next" select="$next"/>
		      </xsl:call-template>

		      <xsl:call-template name="user.footer.navigation"/>
            </body>
          </html>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>-->
	  <div class="titlepage">
	    <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="book.titlepage.before.recto"/>
	    <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="book.titlepage.recto"/>
	    <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="book.titlepage.before.verso"/>
	    <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="book.titlepage.verso"/>
	    <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="book.titlepage.separator"/>
	  </div>
<!--    </xsl:otherwise>
  </xsl:choose>-->
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="*" mode="book.titlepage.recto.mode">
  <!-- if an element isn't found in this mode, -->
  <!-- try the generic titlepage.mode -->
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="titlepage.mode"/>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="*" mode="book.titlepage.verso.mode">
  <!-- if an element isn't found in this mode, -->
  <!-- try the generic titlepage.mode -->
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="titlepage.mode"/>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="title" mode="book.titlepage.recto.auto.mode">
<div use-attribute-sets="book.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="book.titlepage.recto.mode"/>
</div>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="subtitle" mode="book.titlepage.recto.auto.mode">
<div use-attribute-sets="book.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="book.titlepage.recto.mode"/>
</div>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="author" mode="book.titlepage.recto.auto.mode">
<div use-attribute-sets="book.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="book.titlepage.recto.mode"/>
</div>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="publisher" mode="book.titlepage.recto.auto.mode">
<div use-attribute-sets="book.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="book.titlepage.recto.mode"/>
</div>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="copyright" mode="book.titlepage.recto.auto.mode">
<div use-attribute-sets="book.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="book.titlepage.recto.mode"/>
</div>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="legalnotice" mode="book.titlepage.recto.auto.mode">
<div use-attribute-sets="book.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="book.titlepage.recto.mode"/>
</div>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="revhistory" mode="book.titlepage.recto.auto.mode">
<div use-attribute-sets="book.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="book.titlepage.recto.mode"/>
</div>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="revision" mode="book.titlepage.recto.auto.mode">
<div use-attribute-sets="book.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="book.titlepage.recto.mode"/>
</div>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="releaseinfo" mode="book.titlepage.recto.auto.mode">
<div use-attribute-sets="book.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="book.titlepage.recto.mode"/>
</div>
</xsl:template>
<!--
<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="abstract" mode="book.titlepage.recto.auto.mode">
<div use-attribute-sets="book.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="book.titlepage.recto.mode"/>
</div>
</xsl:template>
-->

  

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="article.titlepage.recto">
    
  <xsl:choose xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:when xmlns:xsl="http://www.w3.org/1999/XSL/Transform" test="articleinfo/title">
      <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="articleinfo/title"/>
    </xsl:when>
    <xsl:when xmlns:xsl="http://www.w3.org/1999/XSL/Transform" test="artheader/title">
      <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="artheader/title"/>
    </xsl:when>
    <xsl:when xmlns:xsl="http://www.w3.org/1999/XSL/Transform" test="title">
      <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="title"/>
    </xsl:when>
  </xsl:choose>

    
  <xsl:choose xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:when xmlns:xsl="http://www.w3.org/1999/XSL/Transform" test="articleinfo/subtitle">
      <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="articleinfo/subtitle"/>
    </xsl:when>
    <xsl:when xmlns:xsl="http://www.w3.org/1999/XSL/Transform" test="artheader/subtitle">
      <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="artheader/subtitle"/>
    </xsl:when>
    <xsl:when xmlns:xsl="http://www.w3.org/1999/XSL/Transform" test="subtitle">
      <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="subtitle"/>
    </xsl:when>
  </xsl:choose>

    
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="articleinfo/authorgroup"/>
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="artheader/authorgroup"/>
    
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="articleinfo/author"/>
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="artheader/author"/>
    
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="articleinfo/publisher"/>
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="artheader/publisher"/>
    
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="articleinfo/copyright"/>
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="artheader/copyright"/>
    
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="articleinfo/legalnotice"/>
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="artheader/legalnotice"/>
    
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="articleinfo/revhistory"/>
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="artheader/revhistory"/>
    
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="articleinfo/revision"/>
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="artheader/revision"/>
    
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="articleinfo/releaseinfo"/>
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="artheader/releaseinfo"/>
<!--    
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="articleinfo/abstract"/>
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" mode="article.titlepage.recto.auto.mode" select="artheader/abstract"/>
-->  
</xsl:template>
    
    
    
    
    
    
    
    
    
    
    
    
    
  

  

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="article.titlepage.verso">
  
</xsl:template>
  

  

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="article.titlepage.separator">
  
</xsl:template>

  

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="article.titlepage.before.recto">
    <img align="right" src="stylesheet/gnome-logo-large.png" alt="GNOME Logo"/>
  
</xsl:template>

  

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="article.titlepage.before.verso">
  
</xsl:template>


<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="article.titlepage">
<!-- Added to create a separate titlepage. -->
<!--
  <xsl:param name="next" select="."/>
  <xsl:variable name="id">titlepage</xsl:variable>
  <xsl:choose>
    <xsl:when test="$generate.titlepage.link != 0">
	
	  <xsl:variable name="filename">
	    <xsl:call-template name="make-relative-filename">
		  <xsl:with-param name="base.dir" select="$base.dir"/>
		  <xsl:with-param name="base.name" select="concat($id,$html.ext)"/>
		</xsl:call-template>
	  </xsl:variable>
	  
	  <xsl:variable name="title">
	    <xsl:apply-templates select="." mode="title.markup"/>
	  </xsl:variable>
	  
	  <xsl:call-template name="write.chunk">
	    <xsl:with-param name="filename" select="$filename"/>
		<xsl:with-param name="content">
		  <html>
		    <head>
			  <title><xsl:value-of select="$title"/></title>
			</head>
			<body>
			  <xsl:call-template name="body.attributes"/>
			  <xsl:call-template name="user.header.navigation"/>
			  
			  <xsl:call-template name="header.navigation">
			    <xsl:with-param name="next" select="$next"/>
			  </xsl:call-template>
			  
			  <xsl:call-template name="user.header.content"/>
              <div class="titlepage">
                <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="article.titlepage.before.recto"/>
                <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="article.titlepage.recto"/>
                <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="article.titlepage.before.verso"/>
                <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="article.titlepage.verso"/>
                <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="article.titlepage.separator"/>
              </div>
			  <xsl:call-template name="user.footer.content"/>
			  
			  <xsl:call-template name="footer.navigation">
			    <xsl:with-param name="next" select="$next"/>
			  </xsl:call-template>
			  
			  <xsl:call-template name="user.footer.navigation"/>
			</body>
		  </html>
		</xsl:with-param>
	  </xsl:call-template>
    </xsl:when>
	<xsl:otherwise>-->
              <div class="titlepage">
                <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="article.titlepage.before.recto"/>
                <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="article.titlepage.recto"/>
                <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="article.titlepage.before.verso"/>
                <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="article.titlepage.verso"/>
                <xsl:call-template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" name="article.titlepage.separator"/>
              </div>
<!--	</xsl:otherwise>
  </xsl:choose>-->
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="*" mode="article.titlepage.recto.mode">
  <!-- if an element isn't found in this mode, -->
  <!-- try the generic titlepage.mode -->
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="titlepage.mode"/>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="*" mode="article.titlepage.verso.mode">
  <!-- if an element isn't found in this mode, -->
  <!-- try the generic titlepage.mode -->
  <xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="titlepage.mode"/>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="title" mode="article.titlepage.recto.auto.mode">
<div use-attribute-sets="article.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="article.titlepage.recto.mode"/>
</div>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="subtitle" mode="article.titlepage.recto.auto.mode">
<div use-attribute-sets="article.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="article.titlepage.recto.mode"/>
</div>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="authorgroup" mode="article.titlepage.recto.auto.mode">
<div use-attribute-sets="article.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="article.titlepage.recto.mode"/>
</div>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="author" mode="article.titlepage.recto.auto.mode">
<div use-attribute-sets="article.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="article.titlepage.recto.mode"/>
</div>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="publisher" mode="article.titlepage.recto.auto.mode">
<div use-attribute-sets="article.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="article.titlepage.recto.mode"/>
</div>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="copyright" mode="article.titlepage.recto.auto.mode">
<div use-attribute-sets="article.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="article.titlepage.recto.mode"/>
</div>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="legalnotice" mode="article.titlepage.recto.auto.mode">
<div use-attribute-sets="article.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="article.titlepage.recto.mode"/>
</div>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="revhistory" mode="article.titlepage.recto.auto.mode">
<div use-attribute-sets="article.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="article.titlepage.recto.mode"/>
</div>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="revision" mode="article.titlepage.recto.auto.mode">
<div use-attribute-sets="article.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="article.titlepage.recto.mode"/>
</div>
</xsl:template>

<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="releaseinfo" mode="article.titlepage.recto.auto.mode">
<div use-attribute-sets="article.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="article.titlepage.recto.mode"/>
</div>
</xsl:template>
<!--
<xsl:template xmlns:xsl="http://www.w3.org/1999/XSL/Transform" match="abstract" mode="article.titlepage.recto.auto.mode">
<div use-attribute-sets="article.titlepage.recto.style">
<xsl:apply-templates xmlns:xsl="http://www.w3.org/1999/XSL/Transform" select="." mode="article.titlepage.recto.mode"/>
</div>
</xsl:template>
-->

<!-- ***************************Variablelist********************-->

<!-- Custom template to make the term tag bold.-->
<xsl:template match="variablelist//term">
  <xsl:call-template name="inline.boldseq"/>
</xsl:template>

</xsl:stylesheet>
