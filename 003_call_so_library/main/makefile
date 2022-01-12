# build mod
BUILD_MODE=arm64
ifeq ($(BUILD_MODE),x86)
CC=g++
AR=ar
else ifeq ($(BUILD_MODE),arm64)
# all toolchain defines
CC=aarch64-linux-android24-clang++
AR=llvm-ar -target aarch64-linux-android24
else
CC=g++
AR=ar
endif

main:main.o
	$(CC) main.cpp -I ../library -L ../library -lmax -o max

clean:
	rm -rf *.o max

