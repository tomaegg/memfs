build:
	cmake --build build

gen:
	cmake -GNinja -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1

clean:
	rm -rf build

.PHONY: gen clean build
