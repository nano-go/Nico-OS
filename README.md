## Nico OS
A mini and simple os kernel.

## Build and run this

### Prerequsites
1. clang compiler
2. llvm-lld(ld.lld)
3. make
4. qemu-system-i386

You can get `llvm-lld` on the Debian:
``` shell
sudo apt-get install lld
```

### Build
Enter the root directory of this project:
```
make all
```

### Run this project with `QEMU(i386)`
```
make qemu
# make qemu-sdl
# make qemu-gtk
# make qemu-vnc
# make qemu-curses
```

## References
[1] [xv6-public](https://github.com/mit-pdos/xv6-public)  
[2] Remzi H. Arpaci-Dusseau & Andrea C. Arpaci-Dusseau. (2018). *Operating System: Three Easy Pieces*  
[3] 郑钢. 《操作系统真象还原》. 2016.
