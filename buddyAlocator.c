#include"buddyAlocator.h"
 Buddy* buddy;
 HANDLE mutex;
void initBuddy(void* address, int blocksNum)
{
	//1 za kernel
	if (blocksNum == 1) return;

	//inicijalizacija
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
	mutex = Create_mutex();
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
	//losi parametri
	if (address == 0 || blockCnt == 0 || buddy->_freeBlocks+blockCnt>buddy->_numBlocks-1) return;

	int i;
	
	//while (blockCnt > 0) {
		i = higher_log2(blockCnt);

		Element* temp = (Element*)address;
		temp->next = buddy->array[i];
		buddy->array[i] = temp;
		
	//	blockCnt -= pow(2, i);
		buddy->_freeBlocks += pow(2, i);
	//	address = movePointerForBlocks(address, pow(2,i));
		merge(i);
//	}
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
	//ukloni prvi i zameni ga sa sledecim ako postoji
	void* addr = buddy->array[i];
	buddy->array[i] = buddy->array[i]->next;
	return addr;
}

void* Allocate_Block(int blockCnt)
{
	wait(mutex);
	void* b = allocateBlock(blockCnt);
	signal(mutex);
	return b;
}

void Free_Block(void* address, int blockCnt)
{
	wait(mutex);
	freeBlock(address, blockCnt);
	signal(mutex);
	
}

//nalazenje odgovarajuceg elementa i spajanje ako je potrebno
void merge(int i) {
	Element* first = (Element*)buddy->array[i];
	Element* ptr = first->next;
	Element* left = NULL;
	if (ptr == NULL || i>=MAXIMUM_ARRAY_SIZE)
		return;
	
	while (i+1<MAXIMUM_ARRAY_SIZE) {
		//nalazanje korektnog elementa
		Element* correctElement=NULL;
		Element* temp = movePointerForBlocks(buddy->address, 1);
		Element* last=NULL;
		for (int j = 0; j < (int)(buddy->_numBlocks-1/(int)pow(2,i)); j++) {
			if (temp == first)
			{
				if (j % 2 == 0) {
					correctElement = movePointerForBlocks(temp, pow(2, i));
					break;
				}
				else {
					correctElement = last;
					break;
				}
			}
			last = temp;
			temp=movePointerForBlocks(temp, pow(2, i));
		}

		while (ptr != NULL) {
			if (ptr==correctElement)
			{
				break;
				//pronadjen element
			}
			else {
				left = ptr;
				ptr = ptr->next;
			}
		}
			//ne treba spajanje
			if (ptr == NULL)
				return;

				//spoj prvi i drugi
				if (left == NULL) {
					buddy->array[i] = ptr->next;
				}

				//spoj prvi i element koji nije drugi
				else {
					buddy->array[i] = buddy->array[i]->next;
					left->next = ptr->next;
				}

			//propagiranje u sledeci nivo ukoliko 

			first = first < ptr ? first : ptr;		
			first->next =(Element*) buddy->array[i + 1];
			buddy->array[i + 1] =(Element*) first;
			ptr = first->next;
			left =  NULL;
			i++;

	}
}


//void main() {
//	int* addr = malloc(4096 *1000);
//	initBuddy(addr,1000);
//	printBuddies(buddy);
//	printf("\n\n\n");
//
//	void* adr[1000];
//	for (int i = 0; i < 100; i++) {
//		if (i % 3 == 0)
//		{
//			adr[i] = Allocate_Block(1);
//		}
//		if (i % 3 == 1) {
//			adr[i] = Allocate_Block(2);
//		}
//		if (i % 3 == 2) {
//			adr[i] = Allocate_Block(8);
//		}
//	
//	}
//
//	for (int i = 0; i < 100; i++) {
//		Free_Block(adr[i], 1);
//
//	}
//
//	for (int i = 0; i < 100; i++) {
//		if (i % 3 != 0) {
//			adr[i] = movePointerForBlocks(adr[i], 1);
//			Free_Block(adr[i], 1);
//
//		}
//
//	}
//	for (int i = 0; i < 100; i++) {
//		if (i % 3 == 2) {
//			adr[i] = movePointerForBlocks(adr[i], 1);
//			Free_Block(adr[i], 1);
//
//		}
//
//	}
//	for (int i = 0; i < 100; i++) {
//		if (i % 3 == 2) {
//			adr[i] = movePointerForBlocks(adr[i], 1);
//			Free_Block(adr[i], 1);
//
//		}
//
//	}
//
//	for (int i = 0; i < 100; i++) {
//		if (i % 3 == 2) {
//			adr[i] = movePointerForBlocks(adr[i], 1);
//			Free_Block(adr[i], 1);
//
//		}
//
//	}
//	for (int i = 0; i < 100; i++) {
//		if (i % 3 == 2) {
//			adr[i] = movePointerForBlocks(adr[i], 1);
//			Free_Block(adr[i], 1);
//
//		}
//
//	}
//
//	for (int i = 0; i < 100; i++) {
//		if (i % 3 == 2) {
//			adr[i] = movePointerForBlocks(adr[i], 1);
//			Free_Block(adr[i], 1);
//
//		}
//
//	}
//	for (int i = 0; i < 100; i++) {
//		if (i % 3 == 2) {
//			adr[i] = movePointerForBlocks(adr[i], 1);
//			Free_Block(adr[i], 1);
//
//		}
//
//	}
//
//	printBuddies(buddy);
//	printf("\n\n\n");
//
//	}
