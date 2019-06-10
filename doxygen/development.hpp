/**
 * \page development Development and contribution
 *
 * \tableofcontents
 *
 * This page explains how to debug Hydrogen and guides you step by
 * step through the contribution process.
 *
 * It is mainly a combination of the [Getting
 * involved](https://github.com/hydrogen-music/hydrogen/wiki/Getting-involved)
 * page of the Hydrogen wiki and the
 * [DEVELOPERS](https://github.com/hydrogen-music/hydrogen/blob/master/DEVELOPERS)
 * file provided with the application's sources. While it's quite
 * convenient to access the information from within this documentation
 * itself, please make sure you don't miss any updates on the latter
 * file.
 *
 * \section debugging Debugging the source code
 *
 * There are various ways to debug C++ code. But since Hydrogen is a
 * real-time audio application, methods like setting break points and
 * pausing the execution are not the most appropriate ones. Instead, I
 * would recommend to use our log messages, like \ref INFOLOG,
 * instead.
 *
 * \subsection debuggingLog Debugging using log messages
 *
 * The overall idea of debugging code with log messages can be
 * summarized in the following steps:
 *
 * 1. Pin down the erroneous function/context using the information at
 *    hand.
 * 2. Place log messages in the context to display local/global
 *    variables.
 * 3. Recompile the Hydrogen source code (be sure to properly install
 *    the changes using `make install` in the \e build directory when
 *    changing header files).
 *    \code{.sh}
 *      ./build.sh mm
 *    \endcode
 *
 *    Using `./build.sh mm` over changing into the \e build directory
 *    and calling `make` has the advantage of that the former script
 *    is able to invoke `ccache` for you. This helps you to save a lot
 *    of time by recompiling only those parts of the source you
 *    actually touched. 
 * 4. Run Hydrogen from the command line with a verbosity level of
 *    debugging.
 *    \code{.sh}
 *      ./build/src/gui/hydrogen --verbose=Debug
 *    \endcode
 * 5. Acquire further information by reading the logs and continue
 *    with step 1 until you were able to find the exact line of code,
 *    which is causing the trouble.
 *
 * Step 1 is undoubtedly the most difficult of the above steps. If
 * you struggle to pin down a good location in the code base to start
 * the debugging, let me give you three distinct examples and how to
 * approach them.
 *
 * 1. You see some odd error messages in the log of Hydrogen or some
 *    erroneous behavior is accompanied by them. 
 *
 *    Using the log you can figure out which function is the
 *    culprit. Place some log messages inside it to print its private
 *    members (or summary statistics of them) in order to figure out
 *    *what* went wrong. Also place some log messages right before the
 *    culprit gets called by other functions to figure out *where* and
 *    in which context the error occurs.
 *
 * 2. Hydrogen crashes with a segmentation fault and doesn't print any
 *    logs while doing so.
 *
 *    That's a more tricky one. If the crash occurs during startup and
 *    you are not able to see a properly loaded GUI for one or two
 *    seconds, place log messages all over the startup routine (see
 *    \ref startup). Firstly, do it in a quite coarse manner. Then
 *    recompile and execute your code to see which was the last log
 *    message displayed. Remove all previous messages and add some new
 *    ones in a more dense coverage around this very point and so
 *    on. If Hydrogen crashes when performing a specific task, read
 *    into the documentation/code base to figure out which part is
 *    responsible for it and do the same as described above.
 *
 * 3. You experience abnormal behavior without any logs.
 *
 *    These ones are especially hard. If you are able to reproduce the
 *    bug and to boil it down into a minimal example, they get way
 *    more easy to fix. If not, you, unfortunately, have to have some
 *    insights into Hydrogen to get an idea of what might cause the
 *    particular bug in a particular situation and which part of the
 *    code base is covering it. Place some "meaningful" log messages
 *    and queries for member variables in those context and hope to
 *    find something unexpected in the log. But remember, you can
 *    always contact other Hydrogen developers (see \ref
 *    questions). You are not alone.
 *
 * Regardless of the particular nature of the bug, it's always a good
 * idea to check the documentation of related classes and
 * functions. If some particular information you had to look up
 * yourself to understand the code is missing or there is no
 * documentation for the class/function at hand at all, feel
 * encouraged to write some yourself. This will enable other
 * developers and future contributors to be more productive and helps
 * the overall Hydrogen project to evolve.
 *
 * \section codingConventions Coding conventions
 * 
 * We are aware of the fact that not the whole code base uses the same
 * conventions at the moment.  But we would like to establish the
 * following rules, based on best practices. The numbering has no
 * deeper meaning, its just for referencing the items.
 * 
 * 1. Use the tabs for indentation. Set the tabwidth of your editor to 4.
 * 
 * 2. Allow extra space within parentheses, like this:
 *    \code{.cpp}
 *      while( !done ) {
 *              foo();
 *      }
 *    \endcode
 * 
 * 3. Please don't refactor our code because you just don't like its
 *    style or you think that things could be just a little bit better
 *    if reformat the code to fit your style.
 * 
 * 4. Use curly braces for all `if` statements, even one liners. We
 *    don't need to minimize the lines of code.
 * 
 *    Good: 
 *    \code{.cpp}
 *      if( a ){
 *         doB();
 *      }
 *    \endcode
 * 
 *    Error prone: 
 *    \code{.cpp}
 *      if( a ) doB();
 *    \endcode
 *
 *    or
 *    \code{.cpp}
 *      if( a )
 *         doB();
 *    \endcode
 *
 * 5. Method names follow the camel case naming scheme, starting with
 *    a lowercase letter.
 * 
 *    Example: 
 *    \code{.cpp}
 *      void doB( int * myArgument );
 *    \endcode
 * 
 * 6. Use speaking and self-explaining names for your variables
 *    (exception: loop-variables). We don't need to use short likes i,
 *    n, aux etc...
 * 
 * 7. Prepend pointer types with a `p` (for example: `pMySample`),
 *    floats with an `f`, integer types with an `n` and members of a
 *    class with an `m_` (for example: `m_pEngine`).
 * 
 * 8. Use "auto" or range-based for loops to make iteratons on
 *    container classes more readable.
 * 
 * 9. The singleton pattern is quite over-used in Hydrogen. If you're
 *    adding new classes / servers, please take a moment to consider
 *    if it really has to be a singleton.
 * 
 * 10. Maximum line length is 120 chars.
 * 
 * 11. Hydrogen is a cross platform application, which needs to be
 *     build on a great range of architectures and operating systems.
 *     Use only C++11 features and do not use any platform specific
 *     compiler extensions without the need to do so.
 *  
 * \section contribution How to contribute to the code base
 * 
 * This section describes how one can get involved with Hydrogen as a
 * developer. Beside the technical development, you can also help us
 * with other tasks, like maintaing the website, updating/improving
 * documentation and translation, or testing bugfixes. Just contact us
 * via the [mailing
 * list](http://lists.sourceforge.net/mailman/listinfo/hydrogen-devel)
 * if you're interested in such tasks.
 * 
 * \subsection contributionSkill Coding team skills
 * 
 * - C/C++ coding experience
 * - QT5 (used only in the user interface) 
 * 
 * \subsection contributionGithub Get a GitHub account (optional)
 * 
 * Creating a GitHub account is free, allows you to more
 * conveniently share your code with, and enables you to directly help
 * other users with their problems related to Hydrogen.
 * 
 * \subsection contributionSource Download the latest sources
 * 
 * Open a terminal directory and download the latest code from our
 * GitHub page via
 * 
 * \code{.sh}
 *   git clone git://github.com/hydrogen-music/hydrogen.git
 * \endcode
 * 
 * \subsection contributionML Join the mailing list [and forum]
 * 
 * Development and documentation information flows back and forth on
 * the [mailing
 * list](http://lists.sourceforge.net/mailman/listinfo/hydrogen-devel),
 * so you'll want to be subscribed.
 *
 * In the past we also hosted a forum for the Hydrogen community. But
 * due to the lack of time for both maintenance and moderation, it is
 * not available anymore. If you think providing at least one of these
 * could be your way of contributing to the project, get in touch with
 * us. We all would love to have a forum again!
 * 
 * \subsection contributionBug Find a bug
 * 
 * Yes, Hydrogen has bugs. It's pretty solid but try some edge cases
 * or things that other developers may not have tried and it might
 * fall apart, gives you a wrong error message or do some strange
 * things (see \ref debugging for details). Such a thing is a bug and
 * somebody needs to fix it.
 * 
 * If you find one, please create a bug report on our [GitHub
 * bugtracker](https://github.com/hydrogen-music/hydrogen/issues?state=open).
 * If you can not come up with your own bug, don't worry. There are
 * still quite a number of bugs in the bugtracker that need
 * attention. ;)
 * 
 * \subsection contributionBugFix Fix a bug
 * 
 * Simple, huh? Backup the source code then hack one copy around until
 * you've fixed the bug. If you make a complete hash of things restore
 * from your backup and start again, we all do it.
 * 
 * \subsection contributionCC Committing your code
 * 
 * Please take the following things into account:
 * 
 * 1. If you want to send us a bug fix, please include only the
 *    commits which are part of the bug fix. Do **not** mix in new
 *    features or refactor code.
 * 
 * 2. You can reference the GitHub issue number in your pull request
 *    if you want to fix a bug which is already known in our
 *    bugtracker.
 * 
 * \subsection contributionPR Send us a pull request
 * 
 * The easiest way to participate in the development of Hydrogen is to
 * create a fork at
 * [GitHub](https://github.com/hydrogen-music/hydrogen) and create a
 * pull request from your changes. A comprehensive summary of how to
 * do this can be found in our
 * [wiki](https://github.com/hydrogen-music/hydrogen/wiki/How-to-contribute-code-using-Github).
 * 
 * \subsection contributionWait Sit back and wait
 * 
 * It may take a while for someone to try your patch out and commit
 * it. We have lifes and jobs too. But we do our best to get to it at
 * some point.
 * 
 * \subsection contributionOffense Don't take offence
 * 
 * Sometimes patches get rejected. Read why, learn a lessons, try
 * again. It took me three attempts to get my first patch accepted but
 * it was a lot better by take 3 then it had been at the
 * start. (Editor's note: for me it was 3 times as well :))
 * 
 * After you've had a few patches accepted you will be given commit
 * access. There is no hard and fast rule on when this happens but
 * keep submitting good quality patches and it will happen.
 * 
 * \subsection contributionHelp Getting help
 * 
 * Remember that mailing list you joined... (see \ref
 * contributionML). If you're stuck, want to understand why things are
 * done one way and not another or somebody is already working on
 * something similar, if you're having a question, or you just want to
 * tell us how much we rock, send us a mail via
 * hydrogen-devel@lists.sourceforge.net.
 * 
 * Remember, you are not alone. Stay connected with the rest of the
 * development team. Ask questions! Don't get discouraged and don't be
 * shy! We want you to be involved in Hydrogen for a long time to
 * come.
 */
