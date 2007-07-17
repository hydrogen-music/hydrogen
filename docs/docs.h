/**
\defgroup H2CORE Hydrogen Core


\mainpage Hydrogen - Source documentation


Table of contents:
<ul>
<li>\subpage BUILD_HOWTO
<li>\subpage INSTALL_HOWTO
</ul>

*/


/**
\page BUILD_HOWTO Building Hydrogen

\section sec Scaricare i sorgenti
Ottenere i sorgenti dal repository subversion:
\code
svn co http://involution.dyndns.org/svn/involution_game_studio/game_studio game_studio
\endcode


\section linux_build Compilazione su Linux
\subsection linux_ode_build ODE
\code
./configure --enable-double-precision --enable-release --disable-shared
\endcode

\section mac_build Compilazione su Mac OSX
\todo Inserire come compilare ODE su macosx


\section win_build Compilazione su Windows XP
\subsection s ODE
Scaricare e installare "premake" (http://premake.sourceforge.net/)
\code
cd ode-0.8/build
premake --with-tests --target gnu --enable-static-only
make CONFIG=Release
\endcode
\subsection PThreads
E' necessario scaricare pthreads-w32-2-8-0-release.tar.gz da ftp://sourceware.org/pub/pthreads-win32/
\code
cd pthreads-w32-2-8-0-release
make GC
\endcode






*/


/**
\page INSTALL_HOWTO Installing Hydrogen
*/


