aggregate: TbbLogger.h Aggregate.cpp
	g++ -Wall -Wfatal-errors -O3 -g -m32 Aggregate.cpp -o aggregate

win_aggregate: TbbLogger.h Aggregate.cpp
	i686-w64-mingw32-g++ -Wall -Wfatal-errors -O3 -g -m32 Aggregate.cpp -static-libgcc -static-libstdc++ -o aggregate.exe

test: TbbLogger.cpp Test.cpp TbbLogger.h
	g++ -Wall -Wfatal-errors -O3 -g -Ifmt/include/ TbbLogger.cpp Test.cpp posix.o format.o -lpthread -fopenmp -o test

win_test: TbbLogger.cpp Test.cpp TbbLogger.h
	i686-w64-mingw32-g++ -Wall -Wfatal-errors -O3 -g TbbLogger.cpp Test.cpp -static-libgcc -static-libstdc++ -o test.exe

libfmt:
	g++ -Wall -Wfatal-errors -O3 -g -c -Ifmt/include/ fmt/src/*.cc

clean:
	rm -f aggregate aggregate.exe test test.exe posix.o format.o