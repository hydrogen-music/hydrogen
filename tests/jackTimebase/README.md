Integration test checking whether transport and relocation works using the JACK
audio driver in a real life environment.

These tests are basically a reduced version of the audio engine tests suited to
test interaction with the JACK server and other JACK clients. Two instances of
Hydrogen are run and do interact with eachother with at least one of them using
a patch process callback for the JACK driver doing consistency checks.

## Requirements

- `Go` >= 1.20
- `JACK`

Be sure to build the overall project first - e.g. using `./build.sh mm`.

The code is written and tested on **Linux** Devuan. But since the `OSC` package
is written in pure `Go` and all other packages used are part of the standard
library the code _should_ run on **macOS** and **Windows** as well.

## Usage

``` bash
go run main.go
```
