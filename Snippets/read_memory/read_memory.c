/*
 * read_memory.c
 * Description: A simple code snippet to read memory addresses and print the byte array value in a given process
 *
 * Author: Timothy Gan Z.
 * Version: 0.0.1
 * Date: 22 Feb 2018
 *
 * Run format: read_memory.exe <pid> <memory_address> <number_of_bytes_to_read>
 * Example run:	read_memory.exe 7600 048760C8 8
 * Example output: b80f0000
 */

#include <stdio.h>
#include <windows.h>
#include <limits.h>

/**
 * Function: printMemoryAddress
 * 
 * Description: Reads a given memory address and prints the result as a hexadecimal string
 *
 * Input:
 *   The handle to the process whose memory we want to read
 *   The memory address (in hex, e.g. 0x00400000 / 00400000 / 400000)
 *   The number of bytes to read
 */
void printMemoryAddress(HANDLE processHandle, int memoryAddress, int readMemSize){
  // Define and zero the buffer
  unsigned char buffer[readMemSize]; // we need to use unsigned, otherwise numbers such as "B8" with 6 0's behind it (000000B8) becomes printed as "FFFFFFB8" rather than "B8"
  memset(buffer, 0, sizeof(char)*sizeof(buffer[0]));

  // Read the memory and store result in buffer
  ReadProcessMemory(processHandle, (void*)memoryAddress, buffer, sizeof(buffer), NULL);
  
  // Print every character in the buffer as a hexadecimal character
  for (int i = 0; i < sizeof(buffer); i++){
    printf("%02x", buffer[i]); // %02x is to print the leading zeros (vs %x);, otherwise the leading zeros are hidden and doesn't always print 2 characters
  }
}

int main(int argc, char *argv[]){
  // Ensure required argument count is correct
  if(argc != 4){ // 1st argument is always the process name + 3 required arguments
    printf("Error 001: Program needs 3 arguments\nFormat: 'read_memory.exe <pid> <memory_location> <number_of_bytes>', e.g. 'read_memory.exe 7600 00400000 4'");
    return 1;
  }
  
  // Get process arguments
  int processId = strtol(argv[1], NULL, 10);
  int memAddress = (int)strtol(argv[2], NULL, 16); // we get the memory address as hex, so base 16
  int readMemSize = strtol(argv[3], NULL, 10);
  
  // Exit if process arguments are not in correct format
  if(processId == 0 || processId == LONG_MAX || processId == LONG_MIN ||
     memAddress == 0 || memAddress == LONG_MAX || memAddress == LONG_MIN ||
     readMemSize == 0 || readMemSize == LONG_MAX || readMemSize == LONG_MIN){
    printf("%s", "Error 002: Incorrect process arguments.\nFormat: 'read_memory.exe <pid> <memory_location> <number_of_bytes>', e.g. 'read_memory.exe 7600 00400000 4'");
    return 2;
  }
  
  // Get handle of the process requested
  HANDLE processHandle = OpenProcess(PROCESS_VM_READ, FALSE, processId);
  
  // Exit if our process cannot get the handle to the requested process
  if(processHandle == NULL){
    printf("%s", "Error 003: Unable to get handle of pid\nPid either does not exist or something is blocking the OpenProcess call, e.g. lack of permissions.");
    return 3;
  }
  
  // Print the hexadecimal data in the memory address
  printMemoryAddress(processHandle, memAddress, readMemSize);
  
  return 0;
}