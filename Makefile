
#PREFIX      ?= /usr/local
#HELPER_PATH  = $(PREFIX)/lib/afl
#BIN_PATH     = $(PREFIX)/bin

LLVM_CONFIG ?= llvm-config

CFLAGS      ?= -O0 -funroll-loops
CFLAGS      += -w -D_FORTIFY_SOURCE=2 -g -Wno-pointer-sign

CXXFLAGS    ?= -O0 -funroll-loops
CXXFLAGS    += -w -D_FORTIFY_SOURCE=2 -g -Wno-pointer-sign \
               -Wno-variadic-macros

CLANG_CFL    = `$(LLVM_CONFIG) --cxxflags` -Wl, -fno-rtti -fpic $(CXXFLAGS)
CLANG_LFL    = `$(LLVM_CONFIG) --ldflags` $(LDFLAGS)

# User teor2345 reports that this is required to make things work on MacOS X.

ifeq "$(shell uname)" "Darwin"
  CLANG_LFL += -Wl,-flat_namespace -Wl,-undefined,suppress
endif

# CC path
ifeq "$(origin CC)" "default"
  CC         = /usr/local/Cellar/llvm@12/12.0.1_1/bin/clang
  CXX        = /usr/local/Cellar/llvm@12/12.0.1_1/bin/clang++
endif

PROGS      = ./myclangwrapper ./llvm-pass.so ./llvm-rt.o ./llvm-rt-32.o ./llvm-rt-64.o


all: test_deps $(PROGS) all_done

test_deps:

	@echo "[*] Checking for working 'llvm-config'..."
	@which $(LLVM_CONFIG) >/dev/null 2>&1 || ( echo "[-] Oops, can't find 'llvm-config'. Install clang or set \$$LLVM_CONFIG or \$$PATH beforehand."; echo "    (Sometimes, the binary will be named llvm-config-3.5 or something like that.)"; exit 1 )
	@echo "[*] Checking for working '$(CC)'..."
	@which $(CC) >/dev/null 2>&1 || ( echo "[-] Oops, can't find '$(CC)'. Make sure that it's in your \$$PATH (or set \$$CC and \$$CXX)."; exit 1 )
#	@test -f ../afl-showmap || ( echo "[-] Oops, can't find '../afl-showmap'. Be sure to compile AFL first."; exit 1 )
	@echo "[+] All set and ready to build."

./myclangwrapper: myclangwrapper.c | test_deps
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)
	ln -sf myclangwrapper ./myclangwrapper++

./llvm-pass.so: llvm-pass.so.cpp | test_deps
	$(CXX) $(CLANG_CFL) -shared $< -o $@ $(CLANG_LFL)

./llvm-rt.o: llvm-rt.o.c | test_deps
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

./llvm-rt-32.o: llvm-rt.o.c | test_deps
	@printf "[*] Building 32-bit variant of the runtime (-m32)... "
	@$(CC) $(CFLAGS) -m32 -fPIC -c $< -o $@ 2>/dev/null; if [ "$$?" = "0" ]; then echo "success!"; else echo "failed (that's fine)"; fi

./llvm-rt-64.o: llvm-rt.o.c | test_deps
	@printf "[*] Building 64-bit variant of the runtime (-m64)... "
	@$(CC) $(CFLAGS) -m64 -fPIC -c $< -o $@ 2>/dev/null; if [ "$$?" = "0" ]; then echo "success!"; else echo "failed (that's fine)"; fi

all_done:
	@echo "[+] All done! You can now use 'myclangwrapper' to compile programs."

.NOTPARALLEL: clean

clean:
	rm -f $(PROGS) ./myclangwrapper++
