#include"slabUtility.h"


void init_cache(kmem_cache_t* c,const char* name, size_t object_size, void(*ctor)(void*), void(*dtor)(void*)) {
	if (!c)
		return;

	strcpy(c->cache_name, name);
	//c->cache_name = name;

	c->object_size = object_size;
	c->number_of_slabs = 0;
	c->errorCode = 0;
	c->offset = 0;
	
	c->ctor = ctor;
	c->dtor = dtor;
	c->full =  c->partial = c->empty = NULL;
	c->prev = c->next = NULL;
	c->mutex = Create_mutex();
	
	//proracunavanje broja objekata u jednom slabu 
	int val = 0;
	int numObjects = 0;
	while (!numObjects) {
		numObjects = ((int)pow(2,val) * BLOCK_SIZE - sizeof(Slab)) / (object_size+sizeof(int));
		if (numObjects==0)
		val++;

	}
	c->num_objects = numObjects;
	c->fragmentation =((int)pow(2,val) * BLOCK_SIZE - sizeof(Slab)) % (object_size+sizeof(int));

}

void* get_free_object(Slab* s)
{

	int index = s->free;
	s->free = s->array_Free[s->free];
	s->in_Use++;
	kmem_cache_t* cache = s->cache;


	//prazan u pun
	if (s->max_objects==1)
		Remove_Element_fromList_toAnother(&cache->empty, &cache->full, s);


	//prazan postaje poluprazan
	else if (s->in_Use == 1) {
		Remove_Element_fromList_toAnother(&cache->empty, &cache->partial, s);

	}

	//poluprazan postaje pun
	else if (s->in_Use == s->max_objects)
		Remove_Element_fromList_toAnother(&cache->partial,& cache->full, s);

	return	move_pointer_for_size(s->start_address, index * cache->object_size);
	 
}

Slab* create_new_Slab(kmem_cache_t* cache)
{
	
	//postoji prazan ne treba se kreira ili nevalidni argumenti
	if (!cache)
		return 0;
	
	int blck = prepare_number_of_Blocks(sizeof(Slab)+(sizeof(int)+cache->object_size)*cache->num_objects);

	void* address = Allocate_Block(blck);
	if (!address)
		return -1;
	Slab* slab = (Slab*)address;
	

	slab->offset = cache->offset;
	//ukoliko je offset > fragmentation offset ce biti 0 ukoliko je manji bice prethodni offset+CACHE_L1_LINE_SIZE
	cache->offset = cache->offset + CACHE_L1_LINE_SIZE;
	if (cache->offset > cache->fragmentation)
		cache->offset = 0;

	slab->free = 0;
	slab->in_Use = 0;
	slab->max_objects = cache->num_objects;


	slab->next = slab->prev = NULL;
	address = move_pointer_for_size(address, sizeof(Slab));
	slab->array_Free = (int*)address;
	address = move_pointer_for_size(address,slab->offset+cache->num_objects * sizeof(int));
	slab->start_address = address;
	slab->end_address = move_pointer_for_size(address, cache->object_size * cache->num_objects);
	
	for (int i = 0; i < slab->max_objects; i++) {
		slab->array_Free[i] = i + 1;
		if (cache->ctor != NULL) {
			cache->ctor(move_pointer_for_size(address,i*cache->object_size));
		}
		if (i == slab->max_objects - 1)
			slab->array_Free[i] = -1;
	}
	slab->cache = cache;

	cache->number_of_slabs++;
	slab->next = cache->empty;

	if (cache->empty != NULL)
		cache->empty->prev = slab;
	cache->empty = slab;

	return slab;

}

//void change_cache_offset(kmem_cache_t* cache)
//{
//	//ukoliko je offset > fragmentation offset ce biti 0 ukoliko je manji bice prethodni offset+CACHE_L1_LINE_SIZE
//	cache->offset = cache->offset + CACHE_L1_LINE_SIZE;
//	cache->offset = cache->offset * (1 - (cache->offset / cache->fragmentation));
//}

void Remove_Element_fromList_toAnother(Slab** first, Slab** second, Slab* s)
{
	Slab* temp = (*first);
	Slab* old = NULL;
	while (temp != s) {
		old = temp;
		temp = temp->next;
	}
	if (*first == s) {
		(*first) = (*first)->next;

		if (*first!=NULL)
			(*first)->prev = NULL;
	}
	else {
		//if (old!=NULL)
		old->next = s->next;
		if (s->next)
		s->next->prev = old;
	}

	s->next = (*second);
	s->prev = NULL;

	if ((*second) != NULL)
		(*second)->prev = s;
	
	(*second) = s;
}

void free_object(kmem_cache_t* cachep, Slab* slab, void* objp)
{
	//ako je objekat vec slobodan
	int index = (int)((char*)objp - (char*)slab->start_address) / cachep->object_size - 1;
	if (slab->in_Use != slab->max_objects) {
		for (int elem = slab->free;;) {
			if (elem == index)
			{
				//objekat je vec slobodan 
				cachep->errorCode = 2;
				return;
			}
			elem = slab->array_Free[elem];
			if (elem == -1) {
				//objekat nije slobodan i treba ga osloboditi
				break;
			}
		}
	}
	//objekat nije slobodan i treba ga osloboditi
	if (cachep->dtor)
		cachep->dtor(objp);
	if (cachep->ctor)
		cachep->ctor(objp);
	
	if (slab->free != -1) {
		slab->array_Free[index] = slab->free;
		slab->free = index;
	}
	else {
		slab->free = index;
		slab->array_Free[index] = -1;
		//oslobodio se prvi
	}
	slab->in_Use--;

	//slab moze imati samo jedan element pa slab moze biti ili pun ili prazan
	if (slab->in_Use == 0 && slab->max_objects==1)
		Remove_Element_fromList_toAnother(&cachep->full, &cachep->empty, slab);

	//slab je bio pun sada vise nije
	else if (slab->in_Use==slab->max_objects-1)
		Remove_Element_fromList_toAnother(&cachep->full,& cachep->partial, slab);
	
	//slab je bio poluprazan sada je prazan
	else if (slab->in_Use==0)
		Remove_Element_fromList_toAnother(&cachep->partial, &cachep->empty, slab);


}

Slab* find_slab(kmem_cache_t* cachep, void* objp)
{
	Slab* tempSlab = cachep->partial;
	while (tempSlab) {
		if (tempSlab->start_address >= objp && tempSlab->end_address < objp) {
			break;
		}
		tempSlab = tempSlab->next;
	}

	//objekat se nalazi u polupraznom slabu
	if (tempSlab != NULL) {
		return tempSlab;
	}

	tempSlab = cachep->full;
	while (tempSlab) {
		if (tempSlab->start_address >= objp && tempSlab->end_address < objp) {
			break;
		}
		tempSlab = tempSlab->next;
	}

	//objekat se nalazi u punom slabu
	if (tempSlab != NULL) {
		return tempSlab;
	}

	tempSlab = cachep->empty;
	while (tempSlab) {
		if (tempSlab->start_address >= objp && tempSlab->end_address < objp) {
			break;
		}
		tempSlab = tempSlab->next;
	}

	if (tempSlab)
	{
		//objekat je vec oslobodjen
		cachep->errorCode = 2;
		return tempSlab;
	}
	return NULL;
}

int free_cache(kmem_cache_t* cache)
{
	Slab* tempSlab = cache->empty;
	Slab* old = NULL;
	int brBlck=0;
	while (tempSlab) {

		old = tempSlab;
		tempSlab = tempSlab->next;

		cache->number_of_slabs--;
		int blck = prepare_number_of_Blocks(sizeof(Slab) + (sizeof(int) + cache->object_size) * cache->num_objects);
		brBlck += blck;
		void* address = old;
		Free_Block(address, blck);
	}
	return brBlck;

}

double objects_in_use(kmem_cache_t* cache)
{
	double cnt = 0;
	Slab* s = cache->partial;
	while (s) {
		cnt += s->in_Use;
		s = s->next;
	}
	s = cache->full;
	while (s) {
		cnt += s->in_Use;
		s = s->next;
	}
	return cnt;
}
