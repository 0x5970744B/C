/*
 * keyboard_hook.c
 * Description: A crappy global keyboard hooker which prints typed keys to the console
 * Alternative description: My first C/C++ program written the same day I wrote my first C hello world.
 *
 * Compilation: gcc keyboard_hook.c -o keyboard_hook.exe
 *
 * Notes:
 * --- wParam contains the virtual key code: https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx
 * --- lParam contains the keystroke message flags: https://msdn.microsoft.com/en-us/library/windows/desktop/ms644984(v=vs.85).aspx
 *
 * Weakness:
 * --- Program *might* not even be unhooking properly when the program closes
 * --- Program does not currently make use of lParam, so the program cannot detect special characters/keystrokes properly, whether key was held down, whether caps is on, etc
 *
 * Tested working on:
 * --- Windows 10 64-bit (run as admin)
 * --- Windows 10 64-bit (without admin privileges)
 *
 * Author: Timothy Gan Z.
 * Version: 0.0.1
 * Date: 9 Feb 2018
 */


#include <stdio.h>
#include <windows.h>

/* The actual hook is here */
LRESULT CALLBACK keyboardHook(int nCode, WPARAM wParam, LPARAM lParam){
  if ((nCode == HC_ACTION) && ((wParam == WM_SYSKEYDOWN) || (wParam == WM_KEYDOWN))){
    KBDLLHOOKSTRUCT keypress = *((KBDLLHOOKSTRUCT*)lParam);
    int key = keypress.vkCode; //get the virtual key code which was pressed
    putchar(key); //print single keystroke to console
  }
  return CallNextHookEx(NULL, nCode, wParam, lParam); //pass on control to the next hook, if there is any
}

/* Set up the keyboard hook */
void setKeyboardHook(){
  HINSTANCE kernel32Handle = LoadLibrary(TEXT("kernel32.dll")); //we can use any dll as long as it is valid as this is a global hook
  if(kernel32Handle == NULL){
    printf("%s","Error 01: Could not access the dll\n");
	exit(1);
  }

  HHOOK hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHook, kernel32Handle, 0);
  if(!hook){
	FreeLibrary(kernel32Handle);
	printf("%s","Error 02: Failed to set the hook\n");
	exit(2);
  }
  
  /* Looping via GetMessage as recommended by MSDN, which ensures that our hook stays active */
  MSG msg;
  BOOL bRet;
  while( (bRet = GetMessage( &msg, NULL, 0, 0)) != 0){}

  /* Housekeeping - don't think we actually ever get here when we close the program */
  FreeLibrary(kernel32Handle);
  UnhookWindowsHookEx(hook);
}

int main(int argc, char *argv[])
{
  setKeyboardHook();
  return 0;
}