# build mode
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

main:main.o
	$(CC) main.cpp -I ../shared_library -L ../shared_library -lcal -o calc

clean:
	rm -rf *.o calc

