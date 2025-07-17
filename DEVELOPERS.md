# DEVELOPER INFO

1. [Coding Conventions](#coding-conventions)
2. [Commiting Your Code](#commiting-your-code)
3. [Making A Release](#making-a-release)

## Coding Conventions

We are aware of the fact that not the whole codebase uses the same conventions at the moment.
But we would like to establish the following rules, based on best practices. The numbering has no deeper meaning,
its just for referencing the items.

1. Use the tabs for indentation. Set the tabwidth of your editor to `4`.

2. Allow extra space within parentheses, like this:

    ``` c++
    while ( !done ) {
        foo();
    }
    ```

3. Please don't refactor our code because you just don't like its style or you think that things could be
   just a little bit better if reformat the code to fit your style.

4. Use curly braces for all if statements, even one liners. We don't need to minimize the lines of code.

    Good:
    ```c++
    if ( a ) {
        doB();
    }
    ```

    Error prone:
    ```c++
    if( a ) doB();
    
    if ( a )
        doB();
    ```

5. Method names follow the camel case naming scheme, starting with a lowercase letter.

    Example:
    ```c++
    void doB( int * myArgument );
    ```

7. Use speaking and self-explaining names for your variables (exception: loop-variables). We don't need to use short
   likes _i_, _n_, _aux_ etc...

8. Prepend pointer types with a `p` (for example: pMySample), floats with an `f`, integer types with an `n` and 
   members of a class with an `m_` (for example: m_pEngine)

9. Use `auto` or range-based for loops to make iterations on container classes more readable

10. The singleton pattern is quite over-used in Hydrogen. If you're adding new classes / servers, 
   please take a moment to consider if it really has to be a singleton.

11. Maximum line length is 80 chars

12. Hydrogen is a cross platform application, which needs to be build on a great range of architectures and operating systems.
    Use only C++17 features and do not use any platform specific compiler extensions without the need to do so.

## Commiting Your Code

The easiest way to participate in the development of hydrogen is to create a fork at github and create a pull request
from your changes.

Please take the following things into account:

1. If you want to send us a bug fix, please include only the commits which are part of the bug fix. Do *not* mix in new
features or refactor code.

2. You can reference the github issue number in your pull request if you want to fix a bug which is already known in our bugtracker


## Making A Release

Transitioning the code to remove some of the development hooks in
order to make a release has several, easy-to-forget steps.  They are:

  1. Set the version

     a. Configure the correct version in [CMakeLists.txt](CMakeLists.txt)

     b. Update [CHAMGELOG.md](CHAMGELOG.md)

     c. Check if nothing has changed and update version and date in
        [linux/hydrogen.1](linux/hydrogen.1)

     d. Update the `TARGET_VERSION` variable in [.appveyor.yml](.appveyor.yml)
 
     e. In default config file [data/hydrogen.default.conf](data/hydrogen.default.conf)

  2. Run `src/gui/src/about_dialog_contributor_list_update.sh
     GIT_TAG_OF_LAST_RELEASE HEAD` to update the list of recent
     contributors.

  3. Add a corresponding version tag in the documentation and update
     the submodule in [data/doc](data/doc).

  4. Commit your changes.

  5. Create a dedicated branch containing the suffix `-artifacts`, like
     `releases/1.2.2-release-artifacts`, and manually set in `CMakeLists.txt`
     the `IS_DEVEL_BUILD` variable to "false" and ensure `DISPLAY_VERSION` does
     not have a suffix as well as removing the lines from the Windows pipeline
     in `.appveyor` tweaking the `DISPLAY_VERSION` variable according to tag and
     commit id. When pushing this branch to Github, our AppVeyor pipeline will
     create release artifacts for Linux, macOS, and Windows.

  6. Call your friends.  Have a party.  Be sure to tests the artifacts
     on as many systems as possible.  Be sure to install and uninstall
     them, too.

  7. If the release passes these "internal" tests, add new version,
     date, `CHANGELOG.md` updates as well as the release artifacts including
     their SHA256 sums to [linux/org.hydrogenmusic.Hydrogen.metainfo.xml.in](linux/org.hydrogenmusic.Hydrogen.metainfo.xml.in)
     and run `make` in [linux](linux)

  8. Tag the release.  Remember, after tagging the release you may not
     commit changes to the tag.

      ```bash
      git tag -a 0.9.4 -m "Tagging 0.9.4"
      git push --tags origin
      ```

  9. In case the release is a new major or minor version, also create
     a dedicated release branch, like `releases/1.2`.

  10. Create a Github release, incorporating binary packages, source
      package, tag and release announcement.

  11. Upload the release artifacts to
      [SourceForge](https://sourceforge.net/projects/hydrogen/files/Hydrogen/)
      and make them the default download for all supported platforms.

  12. Add the new download links to the `README.md` on the `main` branch as well
      as the Download section of the hydrogen-music.org web page.

  13. Make release announcements on
      - hydrogen-users@lists.sourceforge.net
      - [hydrogen-music.org](https://github.com/hydrogen-music/hydrogen-music)

  14. Update third party repos. Make a PR against:
      - Homebrew: https://github.com/Homebrew/homebrew-cask/blob/master/Casks/hydrogen.rb
      - Flatpak: https://github.com/flathub/org.hydrogenmusic.Hydrogen/
