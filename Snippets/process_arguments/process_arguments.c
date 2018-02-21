/*
 * process_arguments.c
 * Description: A simple code snippet which processes command arguments. It shows how to process dynamic arguments and static argument positions within the process.
 *
 * Author: Timothy Gan Z.
 * Version: 0.0.1
 * Date: 22 Feb 2018
 */

#include <stdio.h>
#include <windows.h>

/**
 * Function: checkProcessArgumentIsSet
 * 
 * Description: check if the current process has been run with a certain argument
 * Generic description: return true if a string has an exact match within a string array, otherwise return false
 *
 * Input:
 *   argc
 *   argv
 *   The string argument we want to check
 *
 * Output:
 *   If there is an exact match within one of the process arguments and the argument we want to check, return true (1)
 *   Otherwise, return false (0)
 */

BOOL checkProcessArgumentIsSet(int argc, char *argv[], char *argument){
  for (int i = 1; i < argc; i++){
    if (strcmp(argv[i], argument) == 0){
      return 1;
    }
  }
  return 0;
}

int main(int argc, char *argv[]){
  // if process has been run with the arguments "-hello", e.g. "process_arguments.exe -hello" or "process_arguments.exe world -hello"
  BOOL x = checkProcessArgumentIsSet(argc, argv, "-hello");
  if(x) printf("%s", "Process is run with argument '-hello'\n");
  
  // if the process has any arguments, print the first one
  if(argv[1]) printf("%s %s", "The first process argument is: ", argv[1]);
  // otherwise say there was no arguments
  else printf("%s", "Process is run without any arguments");
  
  return 0;
}