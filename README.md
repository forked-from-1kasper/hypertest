On Ubuntu 24.04 LTS:

```bash
$ sudo apt install make g++ libglfw3-dev libglew-dev libglm-dev libluajit-5.1-dev libgmp-dev libsqlite3-dev
$ make
$ make run
```

For clang:
```bash
$ make barbarize
$ make all BARBARIZED=true CXX=clang++
$ make run
```

Don’t forget to copy the configuration file:
```bash
$ cp config.lua.example config.lua
```