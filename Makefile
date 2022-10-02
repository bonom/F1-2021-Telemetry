#!/bin/bash

.PHONY: help build clean

help:
	@echo "\nmake build	[ build the capture system ]"
	@echo "make clean	[ clean tmp folder ]"
	@echo "make help	[ show this help and the README.md ]\n"
	@echo "-----------------------------------------------------------------------------"
	cat ./README.md

build:
	@echo "Building capture system..."
	@g++ -std=c++20 capture/main.cpp -o go
	@echo "Done."
	@clear
	@./go

clean:
	@rm -rf ./tmp