c:\mingw32\bin\gcc ../../src/ma95.c -v -Wall -std=c99 -mdll -D_RXMQT -I. -L. -lmqm -lrexx -lrexxapi rxmqt.def -enable-stdcall-fixup -Wl,--output-def=ma95.def -orxmqt.dll 1>gcct.stdout 2>gcct.stderr
