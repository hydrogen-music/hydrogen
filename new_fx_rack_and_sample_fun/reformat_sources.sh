#!/usr/bin/python

import os

def reformat(directory):
	cmd = "astyle --indent=tab --brackets=linux --pad=paren-in"
	os.system("%s %s/*.cpp" % (cmd, directory))
	os.system("%s %s/*.h" % (cmd, directory))
	os.system("rm -f %s/*.orig" % directory)


reformat("libs/hydrogen/include/hydrogen")
reformat("libs/hydrogen/include/hydrogen/fx")
reformat("libs/hydrogen/include/hydrogen/IO")
reformat("libs/hydrogen/include/hydrogen/sampler")
reformat("libs/hydrogen/include/hydrogen/sequencer")
reformat("libs/hydrogen/include/hydrogen/smf")
reformat("libs/hydrogen/include/hydrogen/synth")

reformat("libs/hydrogen/src")
reformat("libs/hydrogen/src/fx")
reformat("libs/hydrogen/src/IO")
reformat("libs/hydrogen/src/sampler")
reformat("libs/hydrogen/src/sequencer")
reformat("libs/hydrogen/src/smf")
reformat("libs/hydrogen/src/synth")
reformat("libs/hydrogen/src/table")


