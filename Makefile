aggregate: TbbLogger.h Aggregate.cpp
	g++ -Wall -Wfatal-errors -O3 -g -Ifmt/include/ Aggregate.cpp ./fmt/src/*.cc -o aggregate

win_aggregate: TbbLogger.h Aggregate.cpp
	i686-w64-mingw32-g++ -Wall -Wfatal-errors -O3 -g -Ifmt/include/ Aggregate.cpp ./fmt/src/*.cc -static-libgcc -static-libstdc++ -o aggregate.exe

test: Test.cpp Test2.cpp TbbLogger.h
	g++ -Wall -Wfatal-errors -O3 -g Test.cpp Test2.cpp -lpthread -fopenmp -o test

win_test: Test.cpp Test2.cpp TbbLogger.h
	i686-w64-mingw32-g++ -Wall -Wfatal-errors -O3 -g Test.cpp Test2.cpp -static-libgcc -static-libstdc++ -o test.exe

clean:
	rm -f aggregate aggregate.exe test test.exe
