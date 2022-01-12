# build mod
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

# -MM to auto analyse its dependecies
CXXFLAGS = -MM 

# AR flags description
# r  - replace existing or insert new file(s) into the archive
# [c] - do not warn if the library had to be created
# [s] - create an archive index (cf. ranlib)
ARFLAGS = r

# all objects defines
srcs:=$(wildcard *.cpp)
objs:=$(srcs:.cpp=.o)


# build script
all:$(objs)
	$(AR) $(ARFLAGS) libstatic.a $(objs) 

%.o:%.cpp
	@echo "Compiling: $< -> $@"
	$(CC) -MMD -c $< -o $@

clean:
	rm *.o *.d *.a
