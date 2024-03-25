Integration tests used to check more complex use-cases we can not cover using
our [unit tests](../src/tests/).

They are not routinely executed without our **AppVeyor** build pipeline or when
calling our [build.sh](../build.sh) script with `t` as argument. But they are,
nevertheless, designed to exit with code `0` on success and with `1` on failure.
