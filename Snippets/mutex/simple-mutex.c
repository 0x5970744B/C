#include <stdio.h>
#include <windows.h>

/**
 * Function: getMutexHandleIfOwner
 * 
 * Description: returns a handle to the requested mutex if we are the owner, otherwise return null
 *
 * Input:
 *   The name of the mutex we want to get a handle on
 *
 * Output:
 *   If we are the owner, the handle to the requested mutex
 *   Otherwise, return NULL
 */
HANDLE getMutexHandleIfOwner(char *mutexName){
  // Create mutex with current thread as mutex owner
  HANDLE mutexHandle = CreateMutex(NULL, TRUE, mutexName);
  if(mutexHandle == NULL){ return NULL; }
  
  // Get the mutex handle within a maximum of 3 seconds
  DWORD waitMutexResult = WaitForSingleObject(mutexHandle, 3000);

  switch(waitMutexResult){
    // Success: got mutex ownership
    case WAIT_OBJECT_0:
      return mutexHandle;
   
    // Failure: the mutex owner was terminated but did not release mutex
    case WAIT_ABANDONED:
      return NULL;
   
    // Failure: did not get reply within the WaitForSingleObject() specified time limit. Most likely mutex is already owned by another thread
    case WAIT_TIMEOUT:
      return NULL;
   
    // Failure: generic Failure
    case WAIT_FAILED:
      return NULL;
  }
}

int main(void)
{
  HANDLE mutexHandle = getMutexHandleIfOwner("example_mutex");
  if(mutexHandle == NULL){
    printf("%s", "This thread is not the mutex owner, exitting\n");
    exit(1);
  }
  
  printf("%s", "This thread is the mutex owner, continue execution\n");
  // Lazy pause
  getchar();
  
  return 0;
}