###############################################################################
#
# INSTALLATION DIRECTORIES
#
# Change this if you want to install somewhere else.

INSTALL_PLUGINS_DIR	=	..\..\data\plugins

###############################################################################
#
# GENERAL
#

CFLAGS		=	$(INCLUDES) -Wall -Werror -O2 -fPIC
CXXFLAGS	=	$(CFLAGS)
PLUGIN_LIB	=	cmt.so

###############################################################################
#
# OBJECT FILES
#

PLUGIN_OBJECTS	=							\
			am.o						\
			ambisonic.o					\
			amp.o						\
			analogue.o					\
			canyondelay.o					\
			cmt.o						\
			descriptor.o					\
			delay.o						\
			dynamic.o					\
			filter.o					\
			freeverb/Components/allpass.o			\
			freeverb/Components/comb.o			\
			freeverb/Components/revmodel.o			\
			freeverb/freeverb.o				\
			grain.o						\
			init.o						\
			lofi.o						\
			mixer.o						\
			noise.o						\
			null.o						\
			organ.o						\
			peak.o						\
			phasemod.o					\
			sine.o						\
			syndrum.o					\
			vcf303.o					\
			wshape_sine.o					\
			hardgate.o					\
			disintegrator.o					\
			pink.o						\
			pink_full.o					\
			pink_sh.o					\
			sledgehammer.o					\
			logistic.o					\

###############################################################################
#
# TARGETS
#

plugin:	$(PLUGIN_LIB)

targets:	$(PLUGIN_LIB)

$(PLUGIN_LIB):	$(PLUGIN_OBJECTS) gcc_exports.map
	$(CXX)	-shared							\
		$(CFLAGS)						\
		-fvisibility=hidden					\
		-fvisibility-inlines-hidden				\
		-s							\
		-Wl,--version-script=gcc_exports.map			\
		-o $(PLUGIN_LIB)					\
		$(PLUGIN_OBJECTS)					

install:	$(PLUGIN_LIB)
	mkdir $(INSTALL_PLUGINS_DIR)
	cp $(PLUGIN_LIB) $(INSTALL_PLUGINS_DIR)

###############################################################################
#
# UTILITIES
#

depend:		$(MACHINE_GENERATED_CODE)
		makedepend `find . -name "*.cpp"` $(INCLUDES)

always:	

clean:
	-rm -f `find . -name "*.o"`
	-rm -f `find . -name "*.so"`
	-rm -f `find .. -name "*~"`
	-rm -f *.bak core score.srt
	-rm -f *.bb *.bbg *.da *-ann gmon.out bb.out
	-rm -f `find .. -name "*.class"`

backup:		clean
	(cd ../../;							\
	tar czvf `date '+../backup/cmt.%Y%m%d%H%M.tgz'` cmt/)

###############################################################################


