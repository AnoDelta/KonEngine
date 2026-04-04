# KonEngine — use CMake for the real build
# This Makefile is a convenience wrapper.

.PHONY: all install clean test tools

all:
	cmake -B build -DCMAKE_BUILD_TYPE=Release
	cmake --build build -j$$(nproc)

install: all
	sudo cmake --install build

tools:
	cmake -B build -DCMAKE_BUILD_TYPE=Release -DKON_BUILD_TOOLS=ON
	cmake --build build -j$$(nproc)

test:
	cmake --build build --target KonTest
	./build/KonTest

clean:
	rm -rf build
