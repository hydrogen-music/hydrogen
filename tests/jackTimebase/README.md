Integration test for the AudioEngine in presence of a JACK Timebase master
(another instance of Hydrogen itself).

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
