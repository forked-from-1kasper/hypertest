On Ubuntu 22.04 LTS:

```bash
$ sudo apt install make g++ libglfw3-dev libglew-dev libglm-dev libluajit-5.1-dev libgmp-dev libsqlite3-dev
$ make
$ make lua
$ make run
```

For clang:
```bash
$ make barbarize
$ make all BARBARIZED=true CXX=clang++
$ make lua
$ make run
```