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
config.o: config.cpp file.hpp
LINIA=g++ -c $< -o $@ -std=c++11 -ggdb

%.o: %.cpp
	$(LINIA)
%.o: %.cpp %
	$(LINIA)
mtlccaw: government.o file.o lua.o main.o json.o config.o
	g++ $^ -o $@  -ggdb -lpthread -llua5.1 -lcrypto -lPocoNet

clean:
	rm *.o || true
	rm mtlccaw || true
gdb: mtlccaw
	gdb mtlccaw
