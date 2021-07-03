# Python Bindings

To generate bindings after a change in hydrogen headers/API,
to install [HydraGen](https://github.com/charbeljc/HydraGen)

```bash
pip3 install git+https://github.com/charbeljc/HydraGen.git
```

Edit include_paths in `h2core.yaml` (sorry, this is work in progress), then
```bash
cd src/bindings
hydragen h2core.yaml .
```
