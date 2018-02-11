/*
 * process_enumeration.c
 *
 * Description:
 * --- Sets SeDebugPrivilege on current process
 * --- Lists processes in "<process_name>	(PID: <pid>)" format
 *
 * Compilation: gcc process_enumeration.c -o process_enumeration.exe -lpsapi
 * Compilation notes:
 * --- Compiled using mingw's gcc, not sure if lpsapi flag if only available in mingw
 * --- Ignored MSDN compilation suggestion to link Psapi.lib as it looks so much more troublesome. The -lpsapi flag solves the issue.
 *
 * Code References:
 * --- MSDN reference for process enumeration: https://msdn.microsoft.com/en-us/library/windows/desktop/ms682623(v=vs.85).aspx
 * --- SO reference for enabling SeDebugPrivilege: https://stackoverflow.com/questions/4590859/system-service-privilege-to-get-process-information-in-windows-7
 *
 * Weakness:
 * --- OpenProcess call fails on system processes on my system, so the program prints the PID correctly but not the filename on system processes. My guess is it is KAV blocking this call, although not sure why something like process explorer doesn't have any issues. Initially thought it was a lack of debug privileges so I created enableDebugPrivilege() which sets the debug privileges token correctly (as long as run as admin), but that did not solve the issue. Other methods such as using GetProcessImageFileName had slightly better results, but still didn't show all the process names so I ignored that.
 *
 * Tested working on:
 * --- Windows 10 64-bit (run as admin in order to get SeDebugPrivilege token set)
 * --- Windows 10 64-bit (without admin privileges will have same results but without the SeDebugPrivilege token set)
 *
 * Author: Timothy Gan Z.
 * Version: 0.0.2
 * Date: 11 Feb 2018
 */

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>

void PrintProcessNameAndID( DWORD processID )
{
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

    // Get a handle to the process.

    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                   PROCESS_VM_READ,
                                   FALSE, processID );

    // Get the process name.

    if (NULL != hProcess )
    {
        HMODULE hMod;
        DWORD cbNeeded;

        if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
             &cbNeeded) )
        {
            GetModuleBaseName( hProcess, hMod, szProcessName, 
                               sizeof(szProcessName)/sizeof(TCHAR) );
        }
    }
	
    // Print the process name and identifier.

    _tprintf( TEXT("%s  (PID: %u)\n"), szProcessName, processID );

    // Release the handle to the process.

    CloseHandle( hProcess );
}

int enableDebugPrivilege(){
	HANDLE currentProcessHandle;
	HANDLE tokenHandle;
	TOKEN_PRIVILEGES tokenPrivileges;
	
	currentProcessHandle = GetCurrentProcess();
	if( !OpenProcessToken(currentProcessHandle, TOKEN_ALL_ACCESS, &tokenHandle) ){
	  //Failed to get token for current process
	  CloseHandle(currentProcessHandle);
	  return 2;
	}
	
	/* Get the LUID for the SeDebugPrivilege privilege.*/
	LookupPrivilegeValue (NULL, SE_DEBUG_NAME, &tokenPrivileges.Privileges[0].Luid);
	
	tokenPrivileges.PrivilegeCount = 1;
    tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	
	// If program fails to adjust privileges -  strangely this succeeds even when it does not actually manage to adjust the privileges (i.e. when not running as admin)
	if( !AdjustTokenPrivileges (tokenHandle, FALSE, &tokenPrivileges, 0, NULL, NULL) ){
	  CloseHandle(currentProcessHandle);
	  CloseHandle(tokenHandle);
	  return 1;
	}
	else{
	  CloseHandle(currentProcessHandle);
	  CloseHandle(tokenHandle);
	  return 0;
	}
}

int main( void )
{
	// This enables SeDebugPrivilege if the program is run as admin
	enableDebugPrivilege();
	
	
    // Get the list of process identifiers.

    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
    {
        return 1;
    }


    // Calculate how many process identifiers were returned.

    cProcesses = cbNeeded / sizeof(DWORD);

    // Print the name and process identifier for each process.

    for ( i = 0; i < cProcesses; i++ )
    {
        if( aProcesses[i] != 0 )
        {
            PrintProcessNameAndID( aProcesses[i] );
        }
    }
	
	//lazy way to pause program
	getchar();

    return 0;
}
