all:
	cp ../static_library/libstatic.a . -v
	g++ -c -fPIC -I ../static_library cal.cpp -o cal.o
	g++ -o libcal.so -Wl,--whole-archive ../static_library/libstatic.a -Wl,--no-whole-archive -shared cal.o

clean:
	rm -f *.o *.so *.a

