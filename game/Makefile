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
