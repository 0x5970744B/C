/*
 * module_enumeration.c
 * Description: A simple code snippet to enumerate all modules for all processes.
 *
 * Compilation: gcc module_enumeration.c -o module_enumeration.exe -lpsapi
 *
 * v0.0.1 Author: Microsoft (https://msdn.microsoft.com/en-us/library/windows/desktop/ms682621(v=vs.85).aspx)
 * > v0.0.1 Author: Timothy Gan Z.
 *
 * Version: 0.0.2
 * Date: 5 March 2018
 *
 * Notes:
 *   The purpose of trying out this code was to seek an alternative method of listing the modules (previously used the snapshot method to list processes) and compare them.
 *   For all processes except one, the "System Process" (PID=0) which the snapshot method could list the modules for, this method of module enumeration worked better or was the same. While most results were similar, there were instances where this method was able to list the modules when the snapshot method did not work.
 *   For example, this method was able to list KAV modules while the snapshot method was blocked.
 *   Using this alternative, we confirm that the module here means the module loaded into memory, and the "Base Address" of the module is the same address printed here, which is what we want.
 *   So the only advantage the snapshot method has is that we can get the base size of each module, although there is probably an alternative way to calculate that.
 *   In other words, if the snapshot method is debugged, my guess is that it is likely it is simply a wrapper function for a bunch of other calls, so we should avoid the snapshot method.
 */

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <psapi.h>

// To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS
// and compile with -DPSAPI_VERSION=1

int PrintModules( DWORD processID )
{
    HMODULE hMods[1024];
    HANDLE hProcess;
    DWORD cbNeeded;
    unsigned int i;

    // Print the process identifier.

    printf( "\nProcess ID: %u\n", processID );

    // Get a handle to the process.

    hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                            PROCESS_VM_READ,
                            FALSE, processID );
    if (NULL == hProcess)
        return 1;

   // Get a list of all the modules in this process.

    if( EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
        for ( i = 0; i < (cbNeeded / sizeof(HMODULE)); i++ )
        {
            TCHAR szModName[MAX_PATH];

            // Get the full path to the module's file.

            if ( GetModuleFileNameEx( hProcess, hMods[i], szModName,
                                      sizeof(szModName) / sizeof(TCHAR)))
            {
                // Print the module name and handle value.

                _tprintf( TEXT("\t%s (0x%08X)\n"), szModName, hMods[i] );
            }
        }
    }
    
    // Release the handle to the process.

    CloseHandle( hProcess );

    return 0;
}

int main( void )
{

    DWORD aProcesses[1024]; 
    DWORD cbNeeded; 
    DWORD cProcesses;
    unsigned int i;

    // Get the list of process identifiers.

    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
        return 1;

    // Calculate how many process identifiers were returned.

    cProcesses = cbNeeded / sizeof(DWORD);

    // Print the names of the modules for each process.

    for ( i = 0; i < cProcesses; i++ )
    {
        PrintModules( aProcesses[i] );
    }

    return 0;
}
