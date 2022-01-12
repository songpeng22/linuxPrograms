BUILD_MODE=arm64
ifeq ($(BUILD_MODE),x86)
CC=g++
AR=ar
else ifeq ($(BUILD_MODE),arm64)
# all toolchain defines
CC=aarch64-linux-android24-clang++
AR=llvm-ar
else
CC=g++
AR=ar
endif

all:
	cp ../static_library/libstatic.a . -v
	$(CC) -c -fPIC -I ../static_library cal.cpp -o cal.o
	$(CC) -o libcal.so -Wl,--whole-archive ../static_library/libstatic.a -Wl,--no-whole-archive -shared cal.o

clean:
	rm -f *.o *.so *.a

