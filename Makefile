sudo apall: mtlccaw
test: mtlccaw
	./mtlccaw system.lua
class.h:
file.o: file.cpp file.hpp
lua.o: lua.cpp class.hpp
main.o: main.cpp file.hpp class.hpp
json.o: json.cpp class.hpp
government.o: government.cpp class.hpp
config.o: config.cpp file.hpp
log.o: log.cpp class.hpp
encodeURI.o: encodeURI.cpp
LINIA=g++ -c $< -o $@ -std=c++11 -ggdb

%.o: %.cpp
	$(LINIA)
%.o: %.cpp %
	$(LINIA)
mtlccaw: government.o file.o lua.o main.o json.o config.o encodeURI.o log.o
	g++ $^ -o $@  -ggdb -lpthread -llua5.1 -lcrypto -lPocoNet

clean:
	rm *.o || true
	rm mtlccaw || true
gdb: mtlccaw
	gdb mtlccaw
tar: mtlccaw
	tar --owner=root --group=root -zcf mtlccaw.tar.gz mtlccaw \
		items.txt accounts.txt aukcje.txt config.txt expowiska.txt \
		system.lua public_html
	tar --owner=root --group=root -zcf margonem_mini.tar.gz mtlccaw.tar.gz install.sh
