CXX=clang++
BUILD=build
CMAKE_ARGS=-DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_CXX_COMPILER=clang++

rebuild: gen
	cmake --build $(BUILD)

build:
	cmake --build $(BUILD)

gen:
	cmake -GNinja -B $(BUILD) $(CMAKE_ARGS)

clean:
	rm -rf $(BUILD)

.PHONY: gen clean build
