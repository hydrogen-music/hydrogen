# Python Bindings

To generate bindings after a change in hydrogen headers/API,
install [HydraGen](https://github.com/charbeljc/HydraGen)

```bash
pip3 install git+https://github.com/charbeljc/HydraGen.git
```

Then, edit include_paths in `h2core.yaml` (sorry, this is work in progress), and finally
```bash
cd src/bindings
hydragen h2core.yaml .
```
