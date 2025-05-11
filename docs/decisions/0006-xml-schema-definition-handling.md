---
status: accepted
date: 2025-05-10
deciders: phil (theGreatWhiteShark)
---

# AD: XML Schema Definition (XSD) handling

## Context and Problem Statement

Since version `0.9.6` of Hydrogen we provide XML schema definitions (XSD) as
part of the `/data` folder installed on the user's system. These XSD files
specify the structure of our drumkits (`drumkit.xml` files), of our
`.h2pattern`, and of our `.h2playlist` files. In case a provided file does not
comply with the installed definition, Hydrogen considers the content to be in
some sort of legacy format and attempt to loaded it using compatibility code in
case of patterns or triggers an upgrade of the XML file in case of drumkits.

However, starting with version `1.2.5` Hydrogen will use the **Qt6** framework
which removed the previous _Qt XML Patterns_ module and dropped the XSD support
along with it.

Do we attempt to port this feature or do we replace it?

## Decision Drivers

* The current XML handling must not be impaired. Legacy detection and drumkit
  upgrade must work at least on the previous level.
* Maintenance should be as simple as possible.

## Considered Options

1. Use a replacement library for XSD parsing and validation, like
   [Xerces](https://xerces.apache.org/xerces-c/).
2. Implement XSD parsing and validation ourselves.
3. Drop and replace existing XSD validation.

## Decision Outcome

Option 2. is off the table. It is too much initial work and too much of a burden
for future maintainers of the project.

I decided to go with option 3. and replace the existing XSD validation.
Primarily, because in most places the decision about which XML format version is
encountered is already done by checking the presence of XML DOM elements instead
of validating against XSD files. This is solely done in the context of patterns
and can be easily replaced in there. In addition, starting with version `2.0` of
Hydrogen, all our custom XML files will include a new element called
`formatVersion` which can be used to uniquely determine the encountered format.

Another reason for dropping XSD validation is that it proved to be error-prone
in the past. In case the user encounters XML files created using newer versions
of Hydrogen - e.g. the downgrading or retrieving files from the internet - the
current definition is most probably the one closest to the future format. But
since the newer one is not compatible, validation will fail and a fallback to
legacy formats will be triggered. A XML DOM element based approach to fallbacks
is more robust in this regard.

However, I would still keep the XSD files and add some tests in the pipeline to
validate our shipped XML file files using them (in order to ensure they do not
diverge from our C++ internals). For local checks of power users and for people
writing external tools to interface with our custom files these are an
accessible source to learn about our XML formats.

### Consequences

* Replace XSD validation based legacy detection in pattern load with XML DOM
  element version.
* Make XML loading methods report back whether they used a legacy fallback and
  base drumkit upgrade on the results.
* Remove the Qt XmlPatterns module (CMake, docs, and source code).
* Add tests using the `xmllint` program in our Linux pipeline to check the
  validity of the shipped XSD files with respect to our shipped XML file.
