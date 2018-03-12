/*
 * deobfuscate.c
 * Description: A string de-obfuscator against a particular dynamic XOR algorithm.
 *
 * Sample: 6DC2A4_xxx
 *
 * Author: Timothy Gan Z.
 * Version: 0.0.1
 * Date: 13 Mar 2018
 */

#include <stdio.h>

void deobfuscate_xor(int loopCounter, char* obfuscatedCharacterArray, int secondLayerXorCharacter){
  loopCounter += 1;
  int loopCounterCopy = loopCounter;
  /* Perform 1st layer XOR de-obfuscation */
  do{
    *(&obfuscatedCharacterArray[1] + loopCounter) ^= *(&obfuscatedCharacterArray[0] + loopCounter);
    --loopCounter;
  }
  while(loopCounter);
  
  /* Perform 2nd layer XOR de-obfuscation */
  obfuscatedCharacterArray[1] ^= secondLayerXorCharacter;
  
  /* Print final de-obfuscated string */
  printf("%s", "String after de-obfuscation: ");
  for ( int i = 0; i < loopCounterCopy; i++ ) {
    printf("%c", *(&obfuscatedCharacterArray[0]+i));
  }
  printf("%s", "\n");
}

int main(void){
  int loopCounter = 0;
  int secondLayerXorCharacter = 0xF6u;
  
  /* String 1 */
  char obfuscatedCharArray_1[1024] = {0,-86,-29,-115,-7,-100,-18,-128,-27,-111,-79,-12,-116,-4,-112,-1,-115,-24,-102,-58,-81,-54,-78,-62,-82,-63,-77,-42,-8,-99,-27,-128,-128}; //"???"
  loopCounter = 31;
  deobfuscate_xor(loopCounter, obfuscatedCharArray_1, secondLayerXorCharacter); // "\Internet Explorer\iexplore.exe"
  
  /* String 2 */
  char obfuscatedCharArray_2[1024] = {0,-67,-40,-86,-60,-95,-51,-2,-52,-30,-122,-22,-122,-122};
  loopCounter = 12;
  deobfuscate_xor(loopCounter, obfuscatedCharArray_2, secondLayerXorCharacter); // "Kernel32.dll"
  
  /* String 3 */
  char obfuscatedCharArray_3[1024] = {0,-75,-57,-94,-61,-73,-46,-126,-16,-97,-4,-103,-22,-103,-40,-40};
  loopCounter = 14;
  deobfuscate_xor(loopCounter, obfuscatedCharArray_3, secondLayerXorCharacter); // "CreateProcessA"
  
  /* String 4 */
  char obfuscatedCharArray_4[1024] = {0,-96,-55,-69,-49,-70,-37,-73,-10,-102,-10,-103,-6,-65,-57,-57};
  loopCounter = 14;
  deobfuscate_xor(loopCounter, obfuscatedCharArray_4, secondLayerXorCharacter); // "VirtualAllocEx"
  
  /* String 5 */
  char obfuscatedCharArray_5[1024] = {0,-95,-45,-70,-50,-85,-5,-119,-26,-123,-32,-109,-32,-83,-56,-91,-54,-72,-63,-63};
  loopCounter = 18;
  deobfuscate_xor(loopCounter, obfuscatedCharArray_5, secondLayerXorCharacter); // "WriteProcessMemory"
  
  /* String 6 */
  char obfuscatedCharArray_6[1024] = {0,-75,-57,-94,-61,-73,-46,-128,-27,-120,-25,-109,-10,-94,-54,-72,-35,-68,-40,-40};
  loopCounter = 18;
  deobfuscate_xor(loopCounter, obfuscatedCharArray_6, secondLayerXorCharacter); // "CreateRemoteThread"
  
  // Lazy Pause
  getchar();

  return 0;
}