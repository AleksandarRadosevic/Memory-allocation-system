
#include"buddyAlocator.h"
#include<stdlib.h>
void initBuddy(void* address, int blocksNum)
{
	//1 is for kernel
	if (blocksNum == 1) return;

	//initialize buddy
	buddy =(Buddy*) address;
	buddy->address = address;
	buddy->_numBlocks=blocksNum;
	buddy->_freeBlocks = blocksNum - 1;
	buddy->_arrSize = higher_log2(blocksNum);
		
	for (int i = 0; i < 32; i++) {
		buddy->array[i] = NULL;
	}

	 
	blocksNum--;
	int i;
	int memFree = movePointerForBlocks(buddy->address, 1);

	while (blocksNum > 0) {
		i = lower_log2(blocksNum);

		buddy->array[i] = (Element*)memFree;
		buddy->array[i]->next = NULL;

		blocksNum = blocksNum - pow(2, i);
		memFree = movePointerForBlocks(memFree, pow(2, i));
		
	}
}

void* allocateBlock(int blockCnt)
{
	if (buddy->_freeBlocks == 0 || buddy->_freeBlocks<blockCnt)
		return NULL;
	int i = higher_log2(blockCnt);
		if (buddy->array[i] != NULL) {
			buddy->_freeBlocks -= (int)pow(2, i);
		return	removeAndReturnElem(i);
		}
		else
		{
			int j = i;
			while (buddy->array[i] == NULL && i<MAXIMUM_ARRAY_SIZE)
				i++;

			//element not found
			if (i == MAXIMUM_ARRAY_SIZE)
				return;
			//found bigger arr
			Element* right = NULL;
			Element* temp = buddy->array[i];
			buddy->array[i] = temp->next;
			
			while (j < i) {
				Element* left = temp;
				left->next = NULL;
				right =(Element*) movePointerForBlocks(temp, pow(2, i) / 2);
				right->next = NULL;
				buddy->array[i - 1] = left;
				i--;
				temp = right;
			}
			buddy->_freeBlocks -= (int)pow(2, i);
			return right;
		}

}

void freeBlock(void *address,int blockCnt)
{
	//invalid arguments
	if (address == 0 || blockCnt == 0 || buddy->_freeBlocks+blockCnt>buddy->_numBlocks) return;

	int i;
	//no merge needed
	while (blockCnt > 0) {
		i = lower_log2(blockCnt);

		Element* temp = (Element*)address;
		temp->next = buddy->array[i];
		buddy->array[i] = temp;
		
		blockCnt -= pow(2, i);
		buddy->_freeBlocks += pow(2, i);
		address = movePointerForBlocks(address, pow(2,i));
		merge(i);
	}
}

void printBuddies(Buddy* buddy)
{
	for (int i = 0; i < buddy->_arrSize; i++) {
		Element* elem = buddy->array[i];
		while (elem != NULL) {		
			printf("Buddy[%d]=%d\n", i, elem);
			elem = elem->next;
		}
	}
}


void* removeAndReturnElem(int i) {
	//remove first elem and replace first with second if exists
	void* addr = buddy->array[i];
	buddy->array[i] = buddy->array[i]->next;
	return addr;
}


void merge(int i) {
	Element* first = (Element*)buddy->array[i];
	Element* ptr = first->next;
	Element* left = NULL;
	if (ptr == NULL || i>=MAXIMUM_ARRAY_SIZE)
		return;
	Element* addr=NULL;
	Element* leftAddr=NULL;
	//if same address then merge
	while (i+1<MAXIMUM_ARRAY_SIZE) {
		while (ptr != NULL) {
			if (movePointerForBlocks(first, pow(2, i)) == ptr || movePointerForBlocks(ptr, pow(2, i))==first) {
				//element can be merged with greater and smaller 
				if (addr == NULL) {
					addr = ptr;
					leftAddr = left;
				}
				else
				{
					// if addr==3 and ptr==1 and first==2 merge ptr and first 
					if (addr > ptr) {
						addr = ptr;
						leftAddr = left;
					}
					break;
				}

			}
			else {
				left = ptr;
				ptr = ptr->next;
			}
		}
			//no merge needed 
			if (addr == NULL)
				return;

				//merge first and second element
				if (leftAddr == NULL) {
					buddy->array[i] = addr->next;
				}

				//merge first and nonsecond element in list
				else {
					buddy->array[i] = buddy->array[i]->next;
					leftAddr->next = addr->next;
				}


			first = first < addr ? first : addr;
			
			first->next =(Element*) buddy->array[i + 1];
			buddy->array[i + 1] =(Element*) first;
			ptr = first->next;
			left = addr = leftAddr = NULL;
			i++;

	}
}

void main() {
	int* addr = malloc(4096 * 32);
	initBuddy(addr, 32);
	printBuddies(buddy);
	printf("\n\n\n");

	void *addr2=allocateBlock(3);
	printBuddies(buddy);
	printf("\n\n\n");

	void* addr4 = movePointerForBlocks(addr2, 3);
	

	freeBlock(addr2, 3);
	printBuddies(buddy);
	printf("\n\n\n");


	freeBlock(addr4, 1);
	printBuddies(buddy);
	printf("\n\n\n");
}
