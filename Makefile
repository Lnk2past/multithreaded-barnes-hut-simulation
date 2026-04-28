.PHONY: all conan build clean dashboard

all: build

CONAN_STAMP = build/conan.stamp

$(CONAN_STAMP): conanfile.py
	conan install . --build=missing
	cmake --preset conan-release
	touch $(CONAN_STAMP)

conan: $(CONAN_STAMP)

build: conan
	cmake --build --preset conan-release
	cmake --install build/Release

clean:
	rm -rf build bin lib

test: build
	rsc-test test.json

dashboard: build
	PYTHONPATH=lib uv run panel serve app --allow-websocket-origin=*
