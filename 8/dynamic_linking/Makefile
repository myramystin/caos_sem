all: main libmath.so

libmath.so: mathlib.cpp
	g++ -shared -fPIC -o libmath.so mathlib.cpp

main: main.cpp
	g++ -o main main.cpp -ldl

clean:
	rm -f main libmath.so

test_preload: libmath.so
	LD_PRELOAD=./libmath.so ./main
