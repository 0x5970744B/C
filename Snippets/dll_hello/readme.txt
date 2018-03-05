Files:
1) hello.c
2) dll.c


Steps:
1. Create hello.dll
gcc -shared C:\mingw\src\dll_hello\dll.c -o C:\mingw\src\dll_hello\hello.dll

2. Create exe which makes use of the dll in same directory (the dll created in step#1 has to be in same directory of exe for the program to work properly)
gcc C:\mingw\src\dll_hello\hello.c -o C:\mingw\src\dll_hello\hello.exe


Run EXE:
Program prints "5", which is returned by the DLL's function called "hello"