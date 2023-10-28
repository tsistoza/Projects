# FILES MODIFIED TO MAKE PINTOS WORK:

Following: https://cspracs.wordpress.com/installing-pintos-on-qemu/

1. SET GDBMACROS, in `$HOME/'insert user'/pintos-anon/src/utils/pintos-gdb`, Replace $HOME with actual path of repository
2. In all project directories (/threads, /userprog, etc..), Change file Make.vars, the last line: `SIMULATOR = -qemu`
3. In `$HOME/'insert user'/pintos-anon/src/utils/pintos`, change line 103: from 'bochs' to 'qemu'
4. In `$HOME/'insert user'/pintos-anon/src/utils/pintos`, change line 259: from 'kernel.bin' to path of kernel.bin `$HOME/'insert user'/pintos-anon/src/'project directory'/build/kernel.bin`
5. In `$HOME/'insert user'/pintos-anon/src/utils/Pintos.pm`, change line 362: from 'loader.bin' to absolute path `$HOME/'insert user'/pintos-anon/src/'project directory'/build/loader.bin`.