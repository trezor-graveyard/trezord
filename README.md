trezord
=======

TREZOR Communication Daemon aka TREZOR Bridge

Checking out sources
--------------------

```
git clone https://github.com/trezor/trezord.git
cd trezord
git submodule update --init
```

Building
--------

Run `build.sh` to build locally.

... or change into `release` directory and run one of the following (requires Docker):

* `make lin32`
* `make lin64`
* `make win32`
* `make win64`

You can also run `make shell` in `release` directory to log into Docker build environment.
