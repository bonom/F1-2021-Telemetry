@ECHO off

cls
del /f /s /q tmp
del start.exe
g++ .\capture\main.cpp -lws2_32 -o start.exe -Wall -std=c++20
.\start.exe