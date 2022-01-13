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
DEFINE=
INCLUDE=-I ./include -I ../shared_library/include -I ../extern/bizwsd/include -I ../extern/cryptopp/include
CXX_FLAGS=-O3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP 

$(info SRCS is ${SRCS})
$(info OBJS is ${OBJS})
$(info DEPS is ${DEPS})

# -lusb-1.0 -lrt
all:$(OBJS) 
	$(CC) -L ../shared_library/ -L /lib/aarch64-linux-gnu -L ../extern/libusb/lib -lbizlars -lusb -o adctest.x $(OBJS) 
%.o:%.cpp
	@echo "Compiling: $< -> $@"
	$(CC) $(INCLUDE) $(CXX_FLAGS) -MF $@ -MT $@ -o $@ $<

clean:
	rm -f src/*.o src/*.d adctest.x

