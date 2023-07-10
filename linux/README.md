## Packagers

**Packagers** should take the auto-generated
`org.hydrogenmusic.hydrogen.metainfo.xml` and
`org.hydrogenmusic.hydrogen.releases.xml` files.

The older `org.hydrogenmusic.Hydrogen.appdata.xml` file is kept for
backward compatibility of existing packaging workflows (at least for
now). But in general the newer and properly licensed
`org.hydrogenmusic.hydrogen.metainfo.xml` should be used.

## Translators

**Translators** can edit the `*.po` files in [po](./po). In case your
language is not present yet, copy the `po/appstream.pot` file to a
file in the [po](./po) folder having your language code as basename
and `.po` as extension, like `es.po` or `pt_BR.po`. Please ensure no
other characters than the [language
code](https://www.iana.org/assignments/language-subtag-registry/language-subtag-registry)
are contained in the basename as the toolchain is using it for the
`xml:lang` tag in the resulting document.

Applying new translations after editing or creating a `.po` file is
done using

``` bash
make
```

But it is also OK to just make a [pull
requests](https://github.com/hydrogen-music/hydrogen/pulls) with the
updated `.po` files or send it via our mailing list
hydrogen-devel[AT]lists.sourceforge.net and let the maintainers do all
the artifact generation for you.

## Maintainers and Developers

**Maintainers** and **Developers** must always edit
`org.hydrogenmusic.hydrogen.metainfo.xml.in` or
`org.hydrogenmusic.hydrogen.releases.xml.in` when adding new releases
or updating meta information.

Translation of these metainfo files is separated from the main
translations in [data/i18n](../data/i18n) because it uses a different
toolchain and the main one should be as slim as necessary (`itstool`,
`msgfmt`, and `msgmerge` instead of Qt's `lupdate` and `lrelease`). In
addition, translation artifacts are not installed on the user's
system.
