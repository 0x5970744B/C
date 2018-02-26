/*
 * virtual_page_info.c
 * Description: A simple code snippet to map out the virtual pages of a process and print out information about each individual virtual page and a summary of the virtual pages
 *
 * Author: Timothy Gan Z.
 * Version: 0.0.1
 * Date: 26 Feb 2018
 *
 * Compilation: gcc virtual_page_info.c -o virtual_page_info.exe -lpsapi
 * Compilation notes:
 * --- Compiled using mingw's gcc, not sure if lpsapi flag is only available in mingw
 * --- Ignored MSDN compilation suggestion to link Psapi.lib as it looks so much more troublesome. The -lpsapi flag solves the issue.
 *
 * Run format: virtual_page_info.exe <pid>
 * Example run:	virtual_page_info.exe
 *
 * * Tested working on:
 * --- Windows 10 64-bit
 * --- Note: This program should be tested on <Win7 to ensure the psapi library is linked correctly. This was compiled on Windows 10, which has PSAPI version 2. We should test running this on a system with PSAPI version 1 as well to ensure it works across all systems. 
 */

#include <stdio.h>
#include <windows.h>
#include <limits.h>

//GetMappedFileName()
#include <Psapi.h>

// Memory structure of each memory block found using VirtualQueryEx
typedef struct _MEMBLOCK
{
    HANDLE hProc; //process handle of the process this memory block is in (seems wasteful to store this multiple times unless we are scanning more than 1 process at a time)
    unsigned char *addr; //pointer to hexadecimal address this memory block starts in
    int size; //size of this memory block

    struct _MEMBLOCK *next; //using linked list: link to next item
} MEMBLOCK;

/**
 * Function: create_memblock
 * 
 * Description: Creates an individual block of memory that stores data of each individual virtual page. This function was a cut&paste from the memory scanner code, and then cut down to remove unused portions. Over here we do not actually currently need to use a separately created memory block, but we continue to do so as in the future a linked list is likely to be used.
 *
 * Input:
 *   hProc - process handle of the process this memory block is in
 *   *meminfo - a pointer to the Windows MEMORY_BASIC_INFORMATION structure returned by VirtualQueryEx
 *
 * Output:
 *   The created memory block structure
 */
MEMBLOCK* create_memblock (HANDLE hProc, MEMORY_BASIC_INFORMATION *meminfo)
{
  MEMBLOCK *mb = malloc (sizeof(MEMBLOCK));
  if (mb){
      mb->hProc = hProc;
      mb->addr = meminfo->BaseAddress;
      mb->next = NULL;
  }
  return mb;
}

/**
 * Function: memoryProtectionConstant_int2str
 * 
 * Description: When we perform VirtualQueryEx, the MEMORY_BASIC_INFORMATION structure returned contains a DWORD "AllocationProtect" and a DWORD "Protect". These DWORD values can refer to one of many string constants, and this function is to convert the DWORD to the corresponding string constant.
 * Memory protection constants list: https://msdn.microsoft.com/en-us/library/windows/desktop/aa366786(v=vs.85).aspx
 *
 * Input:
 *   memoryProtection - the memoryProtection (AllocationProtect or Protect) part of the MEMORY_BASIC_INFORMATION structure returned by VirtualQueryEx
 *
 * Output:
 *   The corresponding string constant of MEMORY_BASIC_INFORMATION.AllocationProtect or MEMORY_BASIC_INFORMATION.Protect
 */
char* memoryProtectionConstant_int2str(DWORD memoryProtection){
  switch(memoryProtection){
    case 0: return "PERMISSION_DENIED";
    case 0x10: return "PAGE_EXECUTE";
    case 0x20: return "PAGE_EXECUTE_READ";
    case 0x40: return "PAGE_EXECUTE_READWRITE";
    case 0x80: return "PAGE_EXECUTE_WRITECOPY";
    case 0x01: return "PAGE_NOACCESS";
    case 0x02: return "PAGE_READONLY";
    case 0x04: return "PAGE_READWRITE";
    case 0x08: return "PAGE_WRITECOPY";
    case 0x40000000: return "PAGE_TARGETS_INVALID | PAGE_TARGETS_NO_UPDATE";
    case 0x100: return "PAGE_GUARD";
    case 0x200: return "PAGE_NOCACHE";
    case 0x400: return "PAGE_WRITECOMBINE";
    default: return "Unknown memory protection value"; //unsure when this happens
  }
}

/**
 * Function: stateConstant_int2str
 * 
 * Description: When we perform VirtualQueryEx, the MEMORY_BASIC_INFORMATION structure returned contains a DWORD "State". This DWORD value can refer to one of many string constants, and this function is to convert the DWORD to the corresponding string constant.
 * State constants list: https://msdn.microsoft.com/en-us/library/windows/desktop/aa366775(v=vs.85).aspx
 *
 * Input:
 *   state - the State part of the MEMORY_BASIC_INFORMATION structure returned by VirtualQueryEx
 *
 * Output:
 *   The corresponding string constant of MEMORY_BASIC_INFORMATION.State 
 */
char* stateConstant_int2str(DWORD state){
  switch(state){
    case 0x1000: return "MEM_COMMIT";
    case 0x10000: return "MEM_FREE";
    case 0x2000: return "MEM_RESERVE";
    default: return "Unknown State value"; //happens mostly (or only?) when permission denied in memory protection
  }
}

/**
 * Function: typeConstant_int2str
 * 
 * Description: When we perform VirtualQueryEx, the MEMORY_BASIC_INFORMATION structure returned contains a DWORD "Type". This DWORD value can refer to one of many string constants, and this function is to convert the DWORD to the corresponding string constant.
 * Type constants list: https://msdn.microsoft.com/en-us/library/windows/desktop/aa366775(v=vs.85).aspx
 *
 * Input:
 *   type - the Type part of the MEMORY_BASIC_INFORMATION structure returned by VirtualQueryEx
 *
 * Output:
 *   The corresponding string constant of MEMORY_BASIC_INFORMATION.Type 
 */
 char* typeConstant_int2str(DWORD type){
  switch(type){
    case 0x1000000: return "MEM_IMAGE";
    case 0x40000: return "MEM_MAPPED";
    case 0x20000: return "MEM_PRIVATE";
    default: return "Unknown Type value"; //happens mostly (or only?) when permission denied in memory protection
  }
}

/**
 * Function: mapVirtualPages
 * 
 * Description: Get the process handle and map out all virtual pages which we can write to using VirtualQueryEx, creating a linked list memory block structure based on the mapped data
 *
 * Input:
 *   pid - the process identifier to be scanned
 */
MEMBLOCK* mapVirtualPages (unsigned int pid)
{
    MEMBLOCK *mb_list = NULL;
    MEMORY_BASIC_INFORMATION meminfo;
    unsigned char *addr = 0;
    
    // summary information
    int numVirtualPages = 0;
    int totalRegionSize = 0;

    HANDLE hProc = OpenProcess (PROCESS_ALL_ACCESS, FALSE, pid);

    if (hProc)
    {
        while (1)
        {
            // VirtualQueryEx returns 0 when it fails (failing is normal once the address goes out of bounds), e.g. the checks I did showed the system error code 87, 126 (invalid parameter, specified module not found).
            // List of system error codes: https://msdn.microsoft.com/en-us/library/windows/desktop/ms681382(v=vs.85).aspx
            if (VirtualQueryEx (hProc, addr, &meminfo, sizeof(meminfo)) == 0)
            {
                printf("VirtualQueryEx failed, so mapping is finished - error - %d\r\n", GetLastError());
                printf("\nSummary\n-------------------\n");
                printf("Number of virtual pages: %d\n", numVirtualPages);
                printf("Total region size: 0x%x\n", totalRegionSize);
                break;
            }
            
            // summary calculations
            numVirtualPages++;
            totalRegionSize += meminfo.RegionSize;
            
            // find the memory mapped file name, if any (this will happen when the the memory is not in virtual memory, but is temporarily residing in a memory mapped file) --- see https://msdn.microsoft.com/en-us/library/ms810627.aspx for more information
            unsigned char fileNameBuffer[260]; // MAX_PATH = 260 characters, see https://msdn.microsoft.com/en-us/library/aa365247.aspx
            memset(fileNameBuffer, 0, sizeof(fileNameBuffer));
            GetMappedFileName(hProc, meminfo.BaseAddress, fileNameBuffer, sizeof(fileNameBuffer));

            MEMBLOCK *mb = create_memblock (hProc, &meminfo);
            if (mb){
                mb->next = mb_list;
                mb_list = mb;
            }
            addr = (unsigned char*)meminfo.BaseAddress + meminfo.RegionSize;
            printf("Filename: %s\n", fileNameBuffer);
            printf("Base address: 0x%x\n", meminfo.BaseAddress);
            printf("Allocation address: 0x%x\n", meminfo.AllocationBase);
            printf("Memory protection: %s\n", memoryProtectionConstant_int2str(meminfo.AllocationProtect));
            printf("Region size: %d\n", meminfo.RegionSize);
            printf("State: %s\n", stateConstant_int2str(meminfo.State));
            printf("Protect: %s\n", memoryProtectionConstant_int2str(meminfo.Protect));
            printf("Type: %s\n", typeConstant_int2str(meminfo.Type));
            printf("\n");
        }
    }
    else
        printf ("Failed to open process - error - %d\r\n", GetLastError());

    return mb_list;
}


int main(int argc, char *argv[]){
  // Ensure required argument count is correct
  if(argc != 2){ // 1st argument is always the process name + 1 required arguments
	  printf("Error 001: Program needs 1 arguments\nFormat: 'virtual_page_info.exe <pid>'");
    return 1;
  }
  
  // Get process arguments
  int processId = strtol(argv[1], NULL, 10);
  
  // Exit if process arguments are not in correct format
  if(processId == 0 || processId == LONG_MAX || processId == LONG_MIN){
    printf("%s", "Error 002: Incorrect process arguments.\nFormat: 'virtual_page_info.exe <pid>', e.g. 'virtual_page_info.exe 7600'");
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
  mapVirtualPages(processId);
  
  return 0;
}