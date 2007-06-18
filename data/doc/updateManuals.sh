#!/bin/sh

docbook2html -u manual_it.docbook
docbook2html -u manual_en.docbook
docbook2html -u manual_es.docbook
docbook2html -u manual_fr.docbook
docbook2html -u manual_nl.docbook

docbook2html -u tutorial_it.docbook
docbook2html -u tutorial_en.docbook
docbook2html -u tutorial_fr.docbook

