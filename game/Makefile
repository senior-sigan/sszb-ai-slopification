SHELL := /bin/bash
PROJECT := Game

.PHONY: all
all: build run

.PHONY: reload
reload:
	cmake -Bbuild

.PHONY: build
build: reload
	cmake --build build -j 4

.PHONY: clean
clean:
	rm -rf build

.PHONY: run
run:
	./build/$(PROJECT)

.PHONY: reload_web
reload_web:
	emcmake cmake -Bcmake-build-web -DPLATFORM=Web -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXECUTABLE_SUFFIX=".html"

.PHONY: build_web
build_web: reload_web
	cmake --build cmake-build-web -j 4

.PHONY: run_web
run_web:
	python3 -m http.server -d cmake-build-web/ 3000

.PHONY: clean_web
clean_web:
	rm -rf cmake-build-web
