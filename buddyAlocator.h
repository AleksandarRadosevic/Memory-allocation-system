#pragma once
#include"Utility.h"

typedef struct Element {
	struct Element* next;
}Element;

typedef struct Buddy {
	int _numBlocks;
	int _arrSize;
	int _freeBlocks;
	void *address;
	struct Element *array[MAXIMUM_ARRAY_SIZE];
	
}Buddy;
static Buddy* buddy;

void initBuddy(void *address,int blocksNum);
void* allocateBlock(int blockCnt);
void freeBlock(void *address,int blockCnt);
void printBuddies(Buddy* b);
void merge(int i);
void* removeAndReturnElem(int);