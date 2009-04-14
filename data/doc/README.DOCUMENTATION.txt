------------------------------------------------------------------------------
                       H Y D R O G E N          Drum machine
------------------------------------------------------------------------------

HYDROGEN DOCUMENTATION
======================

Contents:

1. Overview
2. Tools, Supported Platforms, and Releases
3. Translators
4. Documentors
5. Developers
6. XML and Validation
7. INSTALL.txt Changelog

1. Overview
-----------

Hydrogen documentation is maintained in DocBook 4.0 (XML) and
translated to HTML before release.  Different translations are managed
through GNU gettext PO files.  When the documentation is generated,
the PO files and the master documentation are merged to create the doc
for that specific language.

This document is split up into the three types of people who may want
to contribute to Hydrogen documentation:  Translators, Documentors,
and Developers.  A section is devoted to each one, individually.

This document assumes that you don't know much, and tries to give you
pointers to the stuff you need to know.

2. Tools, Supported Platforms, and Releases
-------------------------------------------

The following tools are used to maintain and process the
documentation.  Depending on what you're working on, you may not need
all (or any) of these.

    xml2po - Creates and updates the translation templates (*.pot) and
    translation files (*.po) based on the original DocBook source.[1]

    po2xml - Merges the master DocBook source with a translation file
    (*.po) to create a translated DocBook file.[1]

    xmlto - Converts DocBook files to HTML.  (Note that xmlto can
    convert DocBook to much more than just HTML.)[2]

    xmllint - Used to validate the DocBook files against the DocBook
    DTD.[3]

    make - GNU's make utility.

As it happens, these tools are all very easy to install on Linux,
being a part of the core toolchains for KDE, Gnome, Debian, etc.
However, these tools are not as easy to set up on non-Linux platforms.
Furthermore, it is undesireable to add these to the list of build
dependencies for Hydrogen.

Therefore, these files will be processed before making a release of
Hydrogen, and the generated HTML output will become a part of the
distribution.

[1] xml2po and po2xml are part of poxml, which is in the KDE SDK.
    http://www.kde.org/.  The Makefile is set up for the KDE3 version.
[2] xmlto is a convenient front-end to an XSLT processor.
    http://cyberelk.net/tim/software/xmlto/
[3] xmllint is part of libxml http://xmlsoft.org/

3. Translators
--------------

To translate documentation for Hydrogen you will need:

    * To understand XML, and enough DocBook to be dangerous.

    * A PO-file editor.  (Note that a text editor works fine, but a
      translation assistant like KBabel is better.)

    * To be able to read and understand English.

If you don't have all the tools listed in Section 2, that's OK.  Ask
the Hydrogen Developer list and someone there can process files for
you.

To make a new translation of the Hydrogen manual or tutorial, simply
copy the template and get started:

    $ cp manual.pot manual_ja.po

Note that the _ZZ is added, and ZZ is the IANA abbreviation code for
that language.  (E.g. 'ja' is for 'Japanese.')  The registry for the
codes is located here:

    http://www.iana.org/assignments/language-subtag-registry

If a translation already exists, but needs to be updated, you can
update the .po file like this:

    $ touch manual.docbook
    $ make manual_es.po

You can either see the changes using 'diff' or your translation
editor.

When you want to check your translation (and view it in HTML), this
can be done like this:

    $ make manual_ja.html

NOTICE:  Before creating the HTML file, the document will be
validated.  If the document does not validate, you will have to alter
your translation file so that the output is a valid DocBook document.
For more information on Validation, see Section 6.

RULES AND GUIDELINES FOR TRANSLATING:

    * No new content.  New content must be first added to the master
      (English) manual, and then translated to all the other manuals.

    * Maintain the DocBook XML structure as closely as possible.  Do
      not add sections, divide paragraphs, or alter the markup
      significantly.

    * The English translation uses a little humor to try to keep the
      reading interesting.  When this happens, the language and idioms
      being used are very cultural.  Please do *NOT* translate the
      words literally.  Instead, please translate the ideas to your
      culture as you see fit.  We've asked the Documentors to mark
      when this is happening so that you don't miss the joke.  This
      should show up in the PO file.

    * If your culture doesn't like American-style informal writing,
      please feel free to make a humor-less translation.

    * Submit translations to the Hydrogen Developers list
      (hydrogen-devel@lists.sourceforge.net)

4. Documentors
--------------

The master Hydrogen Manual and Tutorial are in English.  All new
content and major revisions shall be done there first.  In order to
work on the documentation, you will need:

    * To know and understand XML and DocBook well.

    * To know and understand Hydrogen well enough to *ask*
      *questions*.

    * An XML editor (note: any text editor will do).

    * An XML validator (e.g. xmllint, xsltproc, Xalan)

    * A way to create PNG images.  (e.g. GIMP, PhotoShop)

What you write is pretty much your own style.  Please *do* keep the
text interesting to read by using wit and a more familiar
conversational style.  If you are using an English pun or expression
that is intended to convey humor, please mark it so that the
translator gets the joke:

    <!-- TRANSLATORS: "Have your squash and eat it, too." This
    combines a well-known cliche ("Have your cake and eat it, too.")
    and a pun on the word "squash."  In English, "squash" is a
    vegetable (and not a very popular one) and "squash" is also a verb
    meaning "to flatten in a destructive way."  For example: "I
    squashed the bug to kill it." -->

(Ahem, don't ask me how I came up with that one.....)

Before submitting (or committing) your changes, please make sure that
your documents validate (see Section 6 below).  Some guidelines:

    * Use double-quotes for all attributes.
          Good:   <foo id="bar"/>
          Bad:    <foo id='bar'/>
      Reason:  po2xml chokes on them.

    * Please make good use of <xref> tags for internal links within
      the document.

    * Since we're using <xref>'s -- if you change an id=".."
      attribute, make sure that you change all its references, too.
      If you create a broken link, the document won't validate.

    * Do not make reference to specific section numbers, figure
      numbers, or titles (e.g. "See section 2.1.3 The Menu Bar").
      Instead, use <xref> tags so that this text will be generated for
      you.

    * For italics, you do not need to set the role="italic" attribute,
      since that is the default.  To get boldface, you must use
      role="bold".
          Example:  <emphasis role="bold">really</emphasis>
          Bad:      <emphasis rold="italic">might</emphasis>

    * For web links, don't write the URL twice.  The processor will do
      that for you, and it makes it more readable.
          Good:     <ulink url="http://www.google.com"/>
          Bad:      <ulink url="http://www.google.com">http://www.google.com</ulink>

    * Don't worry about typesetting in the DocBook documents.  That's
      what XSL and CSS stylesheets are for.  Get the content done, and
      then worry about formatting.  Don't get distracted by stuff like
      section indents or relative font sizes.

    * However, *do* worry about typesetting on pre-formatted tags like
      <screen> and <code> and <literallayout>.  Extra spaces and
      indents in the source document *will* carry through all the way
      to the final document.

5. Developers
-------------

Since we don't want to add xmlto, poxml, xmllint, and the DocBook
DTD's to our normal build requirements: All HTML files need to be
generated and committed before releasing.  After the release, the
generated HTML files can (and should) be deleted.  Do not commit the
generated DocBook documents.

If you are preparing a Hydrogen release, you must have all the tools
listed above so that you can process the documents.  You may also need
to understand DocBook enough to help a translator with validation
issues.  (See Section 6 below.)

The reason for doing it this way is that (as of this writing) xmlto,
poxml, xmllint, and DocBook are not very portable across Linux, Mac,
and Windows.  Nor or they even very portable across different Linux
distributions.  However, the tools are fairly stable on Debian/Ubuntu
-- which most of the current developers are using.

6. XML and Validation
---------------------

You've probably written HTML before, and found it pretty easy.  HTML
is an application of something bigger called SGML.  However, SGML very
difficult to implement reliably.  ("Best viewed on Netscape
Navigator(TM)!!")  Because of this, XML was developed as a replacement
to SGML.  To read more about XML, check out the Wikipedia article:

    http://en.wikipedia.org/wiki/XML

It's very much like HTML except that:

    * The tags are case sensitive.  <IMG> and <img> are the same in
      HTML, but in XML they are considered different.

    * Closing tags are not optional.  In HTML you could start a
      paragraph with a <P>, and then start a new one by putting
      another <P>.  The closing tag </P> is implied.  In XML, you must
      include the closing tag:

          <p>"Knock, knock."</p>
          <p>"Who's there?"</p>

    * Empty tags are like this:  <br/>.  (In HTML they didn't have the
      forward slash.)

If a document follows all the rules of XML, it is called
"Well-Formed."  For example, the following is a well-formed XML
document:

    <?xml version='1.0' encoding='UTF-8'?>
    <ijustmadethistagup>
       <because>It's</because><ok/> to <make it="up">as</make>
       you go</ijustmadethistagup>

But the following is *NOT* a well-formed XML document:

    <?xml version='1.0' encoding='UTF-8'?>
    <ijustmadethistagup>
       <because>It's <ok/> to <make it=up>as</make>
       you go</ijustmadethistagup>

(Can you find the errors?  If you get stumped, feed it to a
validator.)

While it's imperative that documents be well-formed, many documents
(such as DocBook) have a specific structure that must be maintained.
For example, in HTML you should only have paragraphs inside of the
body:

    <body>
       <p>I am the very model of a modern
        major general.</p>
    </body>

But, if I do this, it will still be well-formed XML:

    <p>
       <body>I am the <happy>very</happy> model of a modern
        major general.</body>
    </p>

It is not, however, a Valid HTML document.  The W3C published a DTD
(Document Type Definition) for HTML that clearly specifies that the
former is OK and the latter is degenerate.

When a document is checked against the DTD, it is said to be "Valid"
or "Validated" if it passes all the requirements of the DTD.

Validation is important, because our DocBook source files are going to
be _processed_ by several automatic tools.  These tools know the
structure of DocBook, and are able to generate output based on that.
However, if we feed them an invalid document, the tools may process
the data -- but they probably won't process it *right*.

7. README.DOCUMENTATION.txt Changelog
-------------------------------------

2009-04-09 Gabriel M. Beddingfield <gabriel@teuton.org>
	* Create README.DOCUMENTATION.txt
