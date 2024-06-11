Integration tests used to check more complex use-cases we can not cover using
our [unit tests](../src/tests/).

Most are not routinely executed without our **AppVeyor** build pipeline or when
calling our [build.sh](../build.sh) script with `t` as argument. But they are,
nevertheless, designed to exit with code `0` on success and with `1` on failure.

### jackTearDown

Checks whether Hydrogen does exit cleanly and without crashing when using the
JACK audio driver.

### jackTimebase

Checks whether transport and relocation works using the JACK audio driver in a
real life environment.
