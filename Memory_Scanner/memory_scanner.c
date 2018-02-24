/*
 * memory_scanner.c
 * Description: A simple memory scanner and writer
 *
 * v0.0.1 Author: gimmeamilk (https://www.youtube.com/channel/UCnxW29RC80oLvwTMGNI0dAg)
 * > v0.0.1 Author: Timothy Gan Z.
 *
 * Version: 0.1.0
 * Date: 24 Feb 2018
 *
 * Run format: Run as admin and follow instructions printed
 */

#include <windows.h>
#include <stdio.h>

#define IS_IN_SEARCH(mb,offset) (mb->searchmask[(offset)/8] & (1<<((offset)%8)))
#define REMOVE_FROM_SEARCH(mb,offset) mb->searchmask[(offset)/8] &= ~(1<<((offset)%8));

// Memory structure of each memory block found using VirtualQueryEx
typedef struct _MEMBLOCK
{
    HANDLE hProc; //process handle of the process this memory block is in (seems wasteful to store this multiple times unless we are scanning more than 1 process at a time)
    unsigned char *addr; //pointer to hexadecimal address this memory block starts in
    int size; //size of this memory block
    unsigned char *buffer;

    unsigned char *searchmask;
    int matches; //number of matches to the value we are searching for in this memory block
    int data_size; //data size of the value we are scanning for (i.e. 1 byte, 2 bytes, or 4 bytes)

    struct _MEMBLOCK *next; //using linked list: link to next item
} MEMBLOCK;

// The type of search we are doing
typedef enum 
{
    COND_UNCONDITIONAL, //match on every address (this happens if we use the unknown scan initially)
    COND_EQUALS, //exact match

    COND_INCREASED, //increased value by unknown amount
    COND_DECREASED, //decreased value by unknown amount
} SEARCH_CONDITION;


// Enable or disable a privilege in an access token
// source: http://msdn.microsoft.com/en-us/library/aa446619(VS.85).aspx
BOOL SetPrivilege(
    HANDLE hToken,          // access token handle
    LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
    )
{
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if ( !LookupPrivilegeValue(
			NULL,            // lookup privilege on local system
			lpszPrivilege,   // privilege to lookup
			&luid ) )        // receives LUID of privilege
	{
		printf("LookupPrivilegeValue error: %u\n", GetLastError() );
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnablePrivilege)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;

	// Enable the privilege or disable all privileges.

	if ( !AdjustTokenPrivileges(
		   hToken,
		   FALSE,
		   &tp,
		   sizeof(TOKEN_PRIVILEGES),
		   (PTOKEN_PRIVILEGES) NULL,
		   (PDWORD) NULL) )
	{
		  printf("AdjustTokenPrivileges error: %u\n", GetLastError() );
		  return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

	{
		  printf("The token does not have the specified privilege. \n");
		  return FALSE;
	}

	return TRUE;
}

/**
 * Function: create_memblock
 * 
 * Description: Creates an individual block of memory that stores data of each individual virtual page. 
 *
 * Input:
 *   hProc - process handle of the process this memory block is in
 *   *meminfo - a pointer to the Windows MEMORY_BASIC_INFORMATION structure returned by VirtualQueryEx
 *   data_size - data size of the value we are scanning for (i.e. 1 byte, 2 bytes, or 4 bytes)
 *
 * Output:
 *   The created memory block structure
 */
MEMBLOCK* create_memblock (HANDLE hProc, MEMORY_BASIC_INFORMATION *meminfo, int data_size)
{
    MEMBLOCK *mb = malloc (sizeof(MEMBLOCK));

    if (mb)
    {
        mb->hProc = hProc;
        mb->addr = meminfo->BaseAddress;
        mb->size = meminfo->RegionSize;
        mb->buffer = malloc (meminfo->RegionSize);
        mb->searchmask = malloc (meminfo->RegionSize/8);
        memset (mb->searchmask, 0xff, meminfo->RegionSize/8);
        mb->matches = meminfo->RegionSize;
        mb->data_size = data_size;
        mb->next = NULL;
    }

    return mb;
}

/**
 * Function: free_memblock
 * 
 * Description: Frees the buffer and searchmask inside an individual memory block structure, then frees the memory block.
 *
 * Input:
 *   *mb - a pointer to the memory block to be freed
 */
void free_memblock (MEMBLOCK *mb)
{
    if (mb)
    {
        if (mb->buffer)
        {
            free (mb->buffer);
        }

        if (mb->searchmask)
        {
            free (mb->searchmask);
        }

        free (mb);
    }
}

/**
 * Function: update_memblock
 * 
 * Description: Updates an individual memory block structure based on a given memory scan/search condition
 *
 * Input:
 *   *mb - a pointer to the memory block to be updated
 *   condition - the type of scan to be performed
 *   val - (only used if doing an exact value match new/next scan) the value to be searched for
 */
void update_memblock (MEMBLOCK *mb, SEARCH_CONDITION condition, unsigned int val)
{
    static unsigned char tempbuf[128*1024];
    unsigned int bytes_left;
    unsigned int total_read;
    unsigned int bytes_to_read;
    unsigned int bytes_read;

    if (mb->matches > 0)
    {
        bytes_left = mb->size;
        total_read = 0;
        mb->matches = 0;
    
        while (bytes_left)
        {
            bytes_to_read = (bytes_left > sizeof(tempbuf)) ? sizeof(tempbuf) : bytes_left;
            ReadProcessMemory (mb->hProc, mb->addr + total_read, tempbuf, bytes_to_read, (DWORD*)&bytes_read);
            if (bytes_read != bytes_to_read) break;
    
            if (condition == COND_UNCONDITIONAL)
            {
                memset (mb->searchmask + (total_read/8), 0xff, bytes_read/8);
                mb->matches += bytes_read;
            }
            else
            {
                unsigned int offset;
    
                for (offset = 0; offset < bytes_read; offset += mb->data_size)
                {
                    if (IS_IN_SEARCH(mb,(total_read+offset)))
                    {
                        BOOL is_match = FALSE;
                        unsigned int temp_val;
                        unsigned int prev_val = 0;
    
                        switch (mb->data_size)
                        {
                            case 1:
                                temp_val = tempbuf[offset];
                                prev_val = *((unsigned char*)&mb->buffer[total_read+offset]);
                                break;
                            case 2:
                                temp_val = *((unsigned short*)&tempbuf[offset]);
                                prev_val = *((unsigned short*)&mb->buffer[total_read+offset]);
                                break;
                            case 4:
                            default:
                                temp_val = *((unsigned int*)&tempbuf[offset]);
                                prev_val = *((unsigned int*)&mb->buffer[total_read+offset]);
                                break;
                        }
    
                        switch (condition)
                        {
                            case COND_EQUALS:
                                is_match = (temp_val == val);
                                break;
                            case COND_INCREASED:
                                is_match = (temp_val > prev_val);
                                break;
                            case COND_DECREASED:
                                is_match = (temp_val < prev_val);
                                break;
                            default:
                                break;
                        }
    
                        if (is_match)
                        {
                            mb->matches++;
                        }
                        else
                        {
                            REMOVE_FROM_SEARCH(mb,(total_read+offset));
                        }
                    }
                }
            }
    
            memcpy (mb->buffer + total_read, tempbuf, bytes_read);
    
            bytes_left -= bytes_read;
            total_read += bytes_read;
        }
    
        mb->size = total_read;
    }
}


/**
 * Function: create_scan
 * 
 * Description: Get the process handle and map out all virtual pages which we can write to using VirtualQueryEx, creating a linked list memory block structure based on the mapped data
 *
 * Input:
 *   pid - the process identifier to be scanned
 *   data_size - the number of bytes to be searched for (i.e. 1 byte, 2 byte, or 4 byte types)
 */
MEMBLOCK* create_scan (unsigned int pid, int data_size)
{
    MEMBLOCK *mb_list = NULL;
    MEMORY_BASIC_INFORMATION meminfo;
    unsigned char *addr = 0;

    HANDLE hProc = OpenProcess (PROCESS_ALL_ACCESS, FALSE, pid);
    DWORD error  = GetLastError();

    if (hProc)
    {
        while (1)
        {
            if (VirtualQueryEx (hProc, addr, &meminfo, sizeof(meminfo)) == 0)
            {
                break;
            }
#define WRITABLE (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)
            if ((meminfo.State & MEM_COMMIT) && (meminfo.Protect & WRITABLE))
            {
                MEMBLOCK *mb = create_memblock (hProc, &meminfo, data_size);
                if (mb)
                {
                    mb->next = mb_list;
                    mb_list = mb;
                }
            }
            addr = (unsigned char*)meminfo.BaseAddress + meminfo.RegionSize;
        }
    }
    else
        printf ("Failed to open process - error - %d\r\n", error);

    return mb_list;
}

/**
 * Function: free_scan
 * 
 * Description: Frees all the scan data (which is just a memory block linked list).
 *
 * Input:
 *   *mb_list - a pointer to the start of the memory block linked list
 */
void free_scan (MEMBLOCK *mb_list)
{
    CloseHandle (mb_list->hProc);

    while (mb_list)
    {
        MEMBLOCK *mb = mb_list;
        mb_list = mb_list->next;
        free_memblock (mb);
    }
}

/**
 * Function: update_memblock
 * 
 * Description: Updates all the scan data (which is just a memory block linked list) based on a given memory scan/search condition
 *
 * Input:
 *   *mb_list - a pointer to the start of the memory block linked list
 *   condition - the type of scan to be performed
 *   val - (only used if doing an exact value match new/next scan) the value to be searched for
 */
void update_scan (MEMBLOCK *mb_list, SEARCH_CONDITION condition, unsigned int val)
{
    MEMBLOCK *mb = mb_list;
    while (mb)
    {
        update_memblock (mb, condition, val);
        mb = mb->next;
    }
}

/**
 * Function: dump_scan_info
 * 
 * Description: Print out the entire program's memory in hex to the screen
 *
 * Input:
 *   *mb_list - a pointer to the start of the memory block linked list
 *
 * Notes:
 *   Not very useful since it takes forever to print everything to screen and we can't do anything with it. Probably better to save to disk.
 */
void dump_scan_info (MEMBLOCK *mb_list)
{
    MEMBLOCK *mb = mb_list;

    while (mb)
    {
        int i;
        printf ("0x%08x %d\r\n", mb->addr, mb->size);

        for (i = 0; i < mb->size; i++)
        {
            printf ("%02x", mb->buffer[i]);
        }
        printf ("\r\n");

        mb = mb->next;
    }
}

/**
 * Function: poke
 * 
 * Description: Update a given memory address with a new value
 *
 * Input:
 *   hProc - process handle of the process which the memory address is in
 *   data_size - the data size of the value to be updated (i.e. 1 byte, 2 byte, or 4 byte types)
 *   addr - the hexadecimal memory address to be updated
 *   val - the new value the address is to be updated with
 */
void poke (HANDLE hProc, int data_size, unsigned int addr, unsigned int val)
{
    if (WriteProcessMemory (hProc, (void*)addr, &val, data_size, NULL) == 0)
    {
        printf ("poke failed\r\n");
    }
}

/**
 * Function: peek
 * 
 * Description: View the value of a given memory address
 *
 * Input:
 *   hProc - process handle of the process which the memory address is in
 *   data_size - the data size of the value to be viewed (i.e. 1 byte, 2 byte, or 4 byte types)
 *   addr - the hexadecimal memory address to be viewed
 */
unsigned int peek (HANDLE hProc, int data_size, unsigned int addr)
{
    unsigned int val = 0;

    if (ReadProcessMemory (hProc, (void*)addr, &val, data_size, NULL) == 0)
    {
        printf ("peek failed\r\n");
    }

    return val;
}

/**
 * Function: print_matches
 * 
 * Description: Print out all matches to our search in a particular scan to the screen
 *
 * Input:
 *   *mb_list - a pointer to the start of the memory block linked list
 */
void print_matches (MEMBLOCK *mb_list)
{
    unsigned int offset;
    MEMBLOCK *mb = mb_list;

    while (mb)
    {
        for (offset = 0; offset < mb->size; offset += mb->data_size)
        {
            if (IS_IN_SEARCH(mb,offset))
            {
                unsigned int val = peek (mb->hProc, mb->data_size, (unsigned int)mb->addr + offset);
                printf ("0x%08x: 0x%08x (%d) \r\n", mb->addr + offset, val, val);
            }
        }

        mb = mb->next;
    }
}

/**
 * Function: get_match_count
 * 
 * Description: Print out the number of matches to our search in a particular scan to the screen
 *
 * Input:
 *   *mb_list - a pointer to the start of the memory block linked list
 */
int get_match_count (MEMBLOCK *mb_list)
{
    MEMBLOCK *mb = mb_list;
    int count = 0;

    while (mb)
    {
        count += mb->matches;
        mb = mb->next;
    }

    return count;
}


/**
 * Function: str2int
 * 
 * Description: Utility function --- Convert a string to an integer
 *
 * Input:
 *   *s - the string to be converted. This can be in base 16 format (/0x[0-9a-fA-F]+/) or base 10 format (/\d+/)
 */
unsigned int str2int (char *s)
{
    int base = 10;

    if (s[0] == '0' && s[1] == 'x')
    {
        base = 16;
        s += 2;
    }

    return strtoul (s, NULL, base);
}

/**
 * Function: ui_new_scan
 * 
 * Description: UI function --- Starts the program by asking user for input
 *
 * Output:
 *   MEMBLOCK* - the created scan results (which is just a memory block linked list) based on the user input
 */
MEMBLOCK* ui_new_scan(void)
{
    MEMBLOCK *scan = NULL;
    DWORD pid;
    int data_size;
    unsigned int start_val;
    SEARCH_CONDITION start_cond;
    char s[20];

    while(1)
    {
        printf ("\r\nEnter the pid: ");
        fgets (s,sizeof(s),stdin);
        pid = str2int (s);
        printf ("\r\nEnter the data size: ");
        fgets (s,sizeof(s),stdin);
        data_size = str2int (s);
        printf ("\r\nEnter the start value, or 'u' for unknown: ");
        fgets (s,sizeof(s),stdin);
        if (s[0] == 'u')
        {
            start_cond = COND_UNCONDITIONAL;
            start_val = 0;
        }
        else
        {
            start_cond = COND_EQUALS;
            start_val = str2int (s);
        }

        scan = create_scan (pid, data_size);
        if (scan) break;
        printf ("\r\nInvalid scan");
    }

    update_scan (scan, start_cond, start_val);
    printf ("\r\n%d matches found\r\n", get_match_count(scan));

    return scan;
}

/**
 * Function: ui_poke
 * 
 * Description: UI function --- Continuation after ui_new_scan; Ask the user for further input after the initial scan in order to update a given memory address value
 *
 * Output:
 *   hProc - process handle of the process which the memory address is in
 *   data_size - the data size of the value to be viewed (i.e. 1 byte, 2 byte, or 4 byte types)
 */
void ui_poke (HANDLE hProc, int data_size)
{
    unsigned int addr;
    unsigned int val;
    char s[20];

    printf ("Enter the address: ");
    fgets (s,sizeof(s),stdin);
    addr = str2int (s);

    printf ("\r\nEnter the value: ");
    fgets (s,sizeof(s),stdin);
    val = str2int (s);
    printf ("\r\n");

    poke (hProc, data_size, addr, val);
}

/**
 * Function: ui_run_scan
 * 
 * Description: UI function --- Continuation after ui_new_scan; Continue the scan by asking user for further input after the initial scan
 *
 * Output:
 *   MEMBLOCK* - the created scan results (which is just a memory block linked list) based on the user input
 */
void ui_run_scan(void)
{
    unsigned int val;
    char s[20];
    MEMBLOCK *scan;

    scan = ui_new_scan();

    while (1)
    {
        printf ("\r\nEnter the next value or");
        printf ("\r\n[i] increased");
        printf ("\r\n[d] decreased");
        printf ("\r\n[m] print matches");
        printf ("\r\n[p] poke address");
        printf ("\r\n[n] new scan");
        printf ("\r\n[X] extended options"); //because the initial author decided one char could abbreviate everything...
        printf ("\r\n[q] quit\r\n");

        fgets(s,sizeof(s),stdin);
        printf ("\r\n");

        switch (s[0])
        {
            case 'i':
                update_scan (scan, COND_INCREASED, 0);
                printf ("%d matches found\r\n", get_match_count(scan));
                break;
            case 'd':
                update_scan (scan, COND_DECREASED, 0);
                printf ("%d matches found\r\n", get_match_count(scan));
                break;
            case 'm':
                print_matches (scan);
                break;
            case 'p':
                ui_poke (scan->hProc, scan->data_size);
                break;
            case 'n':
                free_scan (scan);
                scan = ui_new_scan();
                break;
            //get input again for extended options
            case 'X':
                printf ("\r\nEnter the extended option choice");
                printf ("\r\n[md] memory dump");
                fgets(s,sizeof(s),stdin);
                printf ("\r\n");
                
                //print hexadecimal memory dump to screen
                if( strcmp(s, "md\n") == 0 ){ dump_scan_info(scan); }
                
                break;
            case 'q':
                free_scan (scan);
                return;
            default:
                val = str2int (s);
                update_scan (scan, COND_EQUALS, val);
                printf ("%d matches found\r\n", get_match_count(scan));
                break;
        }
    }
}




int main (int argc, char *argv[])
{
    // get process handle
    HANDLE hProc = GetCurrentProcess();

    // get access token of process
    HANDLE hToken = NULL;
    if (!OpenProcessToken(hProc, TOKEN_ADJUST_PRIVILEGES, &hToken))
         printf ("Failed to open access token");

    // set token privileges to SE_DEBUG_NAME to able to access OpenProcess() with PROCESS_ALL_ACCESS
    if (!SetPrivilege(hToken, SE_DEBUG_NAME, TRUE))
        printf ("Failed to set debug privilege");

    ui_run_scan();
    return 0;
}