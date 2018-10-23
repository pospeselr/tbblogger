aggregate: TbbLogger.h Aggregate.cpp
	g++ -Wall -Wfatal-errors -O3 -g -Ifmt/include/ Aggregate.cpp ./fmt/src/*.cc -o aggregate

win_aggregate: TbbLogger.h Aggregate.cpp
	i686-w64-mingw32-g++ -Wall -Wfatal-errors -O3 -g -Ifmt/include/ Aggregate.cpp posix.o format.o -static-libgcc -static-libstdc++ -o aggregate.exe

test: TbbLogger.cpp Test.cpp TbbLogger.h
	g++ -Wall -Wfatal-errors -O3 -g -Ifmt/include/ TbbLogger.cpp Test.cpp ./fmt/src/*.cc -lpthread -fopenmp -o test

libfmt: fmt/src/*.cc fmt/include/fmt/*.h
	g++ -Wall -Wfatal-errors -O3 -g -c -Ifmt/include/ fmt/src/*.cc

win_test: TbbLogger.cpp Test.cpp TbbLogger.h
	i686-w64-mingw32-g++ -Wall -Wfatal-errors -O3 -g -Ifmt/include/ TbbLogger.cpp Test.cpp posix.o format.o -static-libgcc -static-libstdc++ -o test.exe

win_libfmt: fmt/src/*.cc fmt/include/fmt/*.h
	i686-w64-mingw32-g++ -Wall -Wfatal-errors -O3 -g -c -Ifmt/include/ fmt/src/*.cc

clean:
	rm -f aggregate aggregate.exe test test.exe posix.o format.o