/*
 * hello.c
 * Description: A simple program that loads a DLL and calls a function from the dll
 * Compilation: <see readme.txt>
 *
 * Author: Timothy Gan Z.
 * Version: 0.0.1
 * Date: 5 Mar 2018
 */

#include <windows.h>
#include <stdio.h>

typedef int (__cdecl *Procedure)(void);

int main(void){
  Procedure dllFunctionPtr;
  HINSTANCE dllHandle = LoadLibrary(TEXT("hello.dll"));
  if(dllHandle != NULL){
	dllFunctionPtr = (Procedure)GetProcAddress(dllHandle, "hello");
	if(dllFunctionPtr != NULL){
	  int x = dllFunctionPtr();
	  printf("%d",x);
	  return 0;
	}
	else{
	  FreeLibrary(dllHandle);
	  printf("%s","Error 0002: Required function is missing in hello.dll");
      return 2;
	}
  }
  else{
	printf("%s","Error 0001: hello.dll could not be found");
    return 1;
  }
}