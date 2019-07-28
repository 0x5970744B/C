/*
 * Description: Decrypts an encrypted tmp file according to the decryption algorithm in the loader dll. The first 32 bytes of the tmp file are a rotating XOR key which is then used to decrypt the remainder of the tmp file.
 * Compilation: gcc decrypt_sectore04.c -o decrypt_sector_e04.exe
 *
 * Author: Timothy Gan Z.
 * Version: 0.0.1
 * Date: 28 Jul 2019
 *
 * Samples only at https://app.any.run/tasks/33950395-7844-4982-a008-6fe8896335a3/
 *   loader dll sample: F8A1DADB87BB5D8019D016223E836D623E5E4F55E531BA0F1EA96BCC9D31A355 (write.exe -> PROPSYS.dll)
 *   encrypted dll sample: E39AE419C7EC50F94038B63539A638360D605F13BF969C94112BCFB7C928B7F2 (write.exe -> XLdZakA.tmp)
 *   result sample: D7016E9A957FFAFA1B8B55FF5A752E01F69835A1A760A60BB7C8DD8B3BA094A5
 *
 * Modify filename as necessary.
 * Could be further optimized, but ignored since it's fully working.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

int main(){
  int readchar;
  char ch;
  FILE *fp;
  char xorkey[32];
  
  //open the encrypted DLL
  fp = fopen("encrypted.tmp", "rb");
  HANDLE encryptedfile = CreateFile("encrypted.tmp", GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0); //for getting size, a bit repetitive. We HAVE to use CreateFile for GetFileSizeEx, so we need to change the reading bytes process if we want to optimize.
  LARGE_INTEGER size;
  GetFileSizeEx(encryptedfile, &size);
  
  if (fp == NULL)
  {
    printf("err\n");
    perror("Error while opening the file.\n");
    exit(EXIT_FAILURE);
  }
  
  //decrypt the data and save into "saveFile" variable
  int i = 0;
  char saveFile[size.LowPart-sizeof(xorkey)];
  while((readchar = fgetc(fp)) != EOF){ //crappy 1 byte read at a time method
    ch = readchar; //after checking for EOF we can use it as a char
    if(i>=sizeof(xorkey)){
      ch = ch ^ xorkey[i%sizeof(xorkey)];
      saveFile[i-sizeof(xorkey)] = ch;
    }
    else{
      xorkey[i] = ch;
    }
    i++;
  }
  fclose(fp);
  
  //save "saveFile" variable data to "decrypted.dll"
  FILE *file = fopen("decrypted.dll", "wb");
  fwrite(saveFile , 1 , sizeof(saveFile) , file);
  fclose(file);
}
