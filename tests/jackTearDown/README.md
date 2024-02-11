Integration test to check whether Hydrogen does properly exit without crashing
when using the JACK audio driver.

## Requirements

- `hydrogen` - the system-wide installation is used
- `Go` >= 1.20
- `JACK`

The code is written and tested on **Linux** Devuan. But since the `OSC` package
is written in pure `Go` and all other packages used are part of the standard
library the code _should_ run on **macOS** and **Windows** as well.

## Usage

``` bash
go run main.go
```
