aggregate: TbbLogger.h Aggregate.cpp
	mkdir -p bin
	g++ -Wall -Wfatal-errors -O3 -g -Ifmt/include/ Aggregate.cpp ./fmt/src/*.cc -o bin/aggregate

win_aggregate: TbbLogger.h Aggregate.cpp
	mkdir -p bin
	i686-w64-mingw32-g++ -Wall -Wfatal-errors -O3 -g -Ifmt/include/ Aggregate.cpp ./fmt/src/*.cc -static-libgcc -static-libstdc++ -o bin/aggregate.exe

test: Test.cpp Test2.cpp TbbLogger.h
	mkdir -p bin
	g++ -Wall -Wfatal-errors -O3 -g Test.cpp Test2.cpp -lpthread -fopenmp -o bin/test

win_test: Test.cpp Test2.cpp TbbLogger.h
	mkdir -p bin
	i686-w64-mingw32-g++ -Wall -Wfatal-errors -O3 -g Test.cpp Test2.cpp -static-libgcc -static-libstdc++ -o bin/test.exe

clean:
	rm bin/*
