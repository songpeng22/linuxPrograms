main:hello.o fun.o
	g++ -o main hello.o fun.o 

hello.o:hello.cpp
	g++ -c hello.cpp 

fun.o:fun.cpp fun.h
	g++ -c fun.cpp

clean:
	rm -rf *.o main

