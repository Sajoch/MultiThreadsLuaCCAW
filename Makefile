all: mtlccaw
test: mtlccaw
	./mtlccaw
class.h: curl.h
file.o: file.cpp file.hpp
lua.o: lua.cpp class.hpp curl.hpp
main.o: main.cpp file.hpp class.hpp
json.o: json.cpp class.hpp
curl.o: curl.cpp curl.hpp
government.o: government.cpp class.hpp
LINIA=g++ -c $< -o $@ -std=c++11 -ggdb

%.o: %.cpp
	$(LINIA)
%.o: %.cpp %
	$(LINIA)
mtlccaw: government.o file.o lua.o main.o json.o curl.o
	g++ $^ -o $%  -ggdb -lpthread -llua5.1 -lcrypto -lcurl

clean:
	rm *.o || true
	rm mtlccaw || true
gdb: mtlccaw
	gdb mtlccaw
