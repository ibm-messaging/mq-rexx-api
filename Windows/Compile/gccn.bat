c:\mingw32\bin\gcc ../../src/ma95.c -v -Wall -std=c99 -mdll -D_RXMQN -I. -L. -lmqm -lrexx -lrexxapi rxmqn.def -enable-stdcall-fixup -Wl,--output-def=ma95.def -orxmqn.dll 1>gccn.stdout 2>gccn.stderr
