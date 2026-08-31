 This folder contains resources used to translate the Hydrogen - the
 actual application.
 
## Translators
 
To improve or finish a translation, edit the corresponding `.ts`
file. In case your language is not present yet, copy the
`hydrogen_en.ts` file to a file having your [language
code](https://www.iana.org/assignments/language-subtag-registry/language-subtag-registry)
placed after the underscore `_`, like `hydrogen_es.ts` or
`hydrogen_pt_BR.po`.

Applying new translations after editing or creating a `.ts` file is
done using [building Hydrogen from
source](https://github.com/hydrogen-music/hydrogen/blob/master/INSTALL.md).

But it is totally OK to just make a [pull
requests](https://github.com/hydrogen-music/hydrogen/pulls) with the
updated `.ts` files or send it via our mailing list
hydrogen-devel[AT]lists.sourceforge.net and let the maintainers do all
the artifact generation for you.

Also note that some general resources requiring translation as well
can be found in the subfolder [linux](../../linux).

Please note that this branch contains the translations for the `2.X` release
series of Hydrogen. Those for `1.2.X` can be found
[here](https://github.com/hydrogen-music/hydrogen/tree/releases/1.2). To ensure
you do not do things twice, its best to start with one release, create a pull
request, and let Hydrogen's development team worry about merging the translation
back to the other release. Afterwards, you can tackle missing ones.

For more details about the overall translation process see
https://github.com/hydrogen-music/hydrogen/wiki/Translator-instructions.


## Developers

Updating all these resource files is now part of the
[build.sh](../build.sh) script. It can be done explicitly but is also
a step in the general cmake build step. This should ensure that
translations are always representing the current state of the code.
