#pragma once

#define BLOCK_SIZE (4096)
#define MAXIMUM_ARRAY_SIZE (32)
#include<math.h>
#include<stdio.h>
#include<Windows.h>
#include<stdlib.h>
#include<stdbool.h>
#include"slabUtility.h"

//return lower log2 part for number i
int static lower_log2(int i) {
	return (int)floor(log2(i));
}
//return higher log2 part for number i
int static higher_log2(int i) {
	return (int)ceil(log2(i));
}
//move pointer for NumberOfBlocks parameter 
int static movePointerForBlocks(int address,int i) {
	return (int) (address)+BLOCK_SIZE * i;
}

HANDLE static Create_mutex() {
	//rucka se ne nasledjuje (prvi argument) ne pripada nijednog niti(drugi argument) bez imena je (treci argument)
	HANDLE h = CreateMutex(NULL, FALSE, NULL);
	if (!h)
		return 0;
	else return h;
}

void static wait(HANDLE h) {
	WaitForSingleObject(h, INFINITE);
}


 void static signal(HANDLE h) {
	ReleaseMutex(h);
}


 int static prepare_number_of_Blocks(int value) {
	if (value <= 0)
		return 0;
	return (int) ceil((double)value / BLOCK_SIZE);

}

 static void* move_pointer_for_size(void *ptr,int size) {
	ptr = (char*)ptr + size;
	return ptr;

}