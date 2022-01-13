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

# all objects defines
SRCS:=$(wildcard src/*.cpp)
SRCS:=$(filter-out src/dllmain.cpp,$(SRCS))
OBJS:=$(SRCS:.cpp=.o)
DEPS:=$(SRCS:.cpp=.d)
DEFINE=-D_GLIBCXX_USE_NANOSLEEP -DUSE_LIBUSB
# -I /usr/aarch64-linux-gnu/include 
INCLUDE=-I ./include -I ../static_library/include -I ../extern/bizwsd/include -I ../extern/cryptopp/include -I /opt/android-ndk/ndk/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include -I ../extern/libusb/include
CXX_FLAGS=-O3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP 

$(info SRCS is ${SRCS})
$(info OBJS is ${OBJS})
$(info DEPS is ${DEPS})

all:$(OBJS) 
	$(CC) -o libbizlars.so -Wl,--whole-archive ../static_library/libcryptopp.a -Wl,--no-whole-archive -shared $(OBJS) 
%.o:%.cpp
	@echo "Compiling: $< -> $@"
	$(CC) $(DEFINE) $(INCLUDE) $(CXX_FLAGS) -MF $*.d -MT $*.o -o $*.o $<

clean:
	rm -f src/*.o src/*.d *.so *.a

