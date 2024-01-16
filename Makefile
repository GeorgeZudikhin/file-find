all: myfind

myfind: myfind.cpp
	g++ -Wall -Werror -std=c++17 -O -o myfind myfind.cpp

clean:
	rm -f myfind