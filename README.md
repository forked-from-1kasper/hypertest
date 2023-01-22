On Ubuntu 22.04 LTS:

```bash
$ sudo apt install make g++ libglfw3-dev libglew-dev libluajit-5.1-dev libglm-dev libgmp-dev
$ make
$ make fennel
$ make run
```

For clang:
```bash
$ make barbarize
$ make all BARBARIZED=true CXX=clang++
$ make fennel
$ make run
```