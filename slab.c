#include"slabUtility.h"
#include"buddyAlocator.h"

kmem_cache_t *small_cashes;
kmem_cache_t *cache_caches;
kmem_cache_t* last_used = NULL;



void kmem_init(void* space, int block_num) {
	if (!space || block_num<=1)
		return;
	
	initBuddy(space, block_num);

	//kreiranje malih keseva
	int sizeCaches = sizeof(kmem_cache_t) * (NUMBER_OF_SMALL_CASHES + 1);
	int blckSmall = prepare_number_of_Blocks(sizeCaches);

	void* startAddress = Allocate_Block(blckSmall);
	if (!startAddress) return;

	small_cashes = (kmem_cache_t*)startAddress;
	startAddress = move_pointer_for_size(startAddress,sizeof(kmem_cache_t)*NUMBER_OF_SMALL_CASHES);

	for (int i = 0; i < NUMBER_OF_SMALL_CASHES; i++) {
		init_cache(&small_cashes[i], "size-N",(int) pow(2, i + 5), NULL, NULL);
	/*	if (i != 0)
		{
			small_cashes[i - 1].next = &small_cashes[i];
			small_cashes[i].prev = &small_cashes[i - 1];
		}
		*/
	}


	//kreiranje kesa za keseve objekata
	cache_caches = (kmem_cache_t*)startAddress;
	init_cache(cache_caches, "kes objekata", sizeof(kmem_cache_t), NULL,NULL);



}

//kreiranje jednog kesa
kmem_cache_t* kmem_cache_create(const char* name, size_t size,	void (*ctor)(void*),void (*dtor)(void*)) {

	if (size <= 0)
		return 0;
	
	kmem_cache_t* cache =(kmem_cache_t*) kmem_cache_alloc(cache_caches);
	if (!cache) 
		return 0;
	
	init_cache(cache, name, size, ctor, dtor);
	
	cache->next = last_used;
	if (last_used != NULL) {
		last_used->prev = cache;
	}
	last_used = cache;


	return cache;

}

int kmem_cache_shrink(kmem_cache_t* cachep)
{
	if (!cachep)
		return;

	wait(cachep->mutex);
	int b = free_cache(cachep);
	signal(cachep->mutex);
	return b;

}



void* kmem_cache_alloc(kmem_cache_t* cachep) {

	if (!cachep)
		return;
	void* element;

	wait(cachep->mutex);

	//dohvati prvo iz delimicno popunjenog slaba
	if (cachep->partial != NULL)
	{
		element = get_free_object(cachep->partial);
	}
	//dohvati iz praznog slaba
	else if (cachep->empty != NULL) {
		element = get_free_object(cachep->empty);
	}
	//dodaj novi slab
	else {
		Slab* sl =  create_new_Slab(cachep);
		if (!sl) {
			cachep->errorCode = 1;	
			signal(cachep->mutex);
			return NULL;
		}
		element = get_free_object(cachep->empty);

	}
	signal(cachep->mutex);
	return element;


}

void kmem_cache_free(kmem_cache_t* cachep, void* objp)
{
	if (!cachep || !objp) {
		return;
	}
	wait(cachep->mutex);

	Slab* slab = find_slab(cachep, objp);
	if (!slab) {
		//nije pronadjen ni u jednom slabu
		cachep->errorCode = 3;
		signal(cachep->mutex);
		return;
	}
	else if (cachep->errorCode == 2) {
		//vec je oslobodjen
		signal(cachep->mutex);
		return;
	}
	free_object(cachep, slab, objp);

	signal(cachep->mutex);


}

void* kmalloc(size_t size) {

	int chck = higher_log2(size);
	if (chck >= 5 && chck <= 17) {
			kmem_cache_t* cache = &small_cashes[chck-5];
			//wait(cache->mutex);
		void *element=kmem_cache_alloc(cache);
			//signal(cache->mutex);
		return element;
		}
		else {
			printf("Stepen dvojke mora biti u opsegu od 5 do 17!");
		}

	return NULL;
}

void kfree(const void* objp)
{
	if (!objp)
		return;

	for (int i = 0; i < NUMBER_OF_SMALL_CASHES; i++) {
		kmem_cache_t* cache = &small_cashes[i];
		//veliki znak pitanja da li treba komentar
		wait(cache->mutex);
		kmem_cache_free(cache, objp);

		//objekat nije pronadjen
		if (cache->errorCode == 3) {
			cache->errorCode = 0;
			signal(cache->mutex);
		}

		else if (cache->errorCode == 2 || cache->errorCode == 0)
		{
			//objekat je sad oslobodjen ili je vec bio slobodan
			signal(cache->mutex);
			return;
		}
	}
}

void kmem_cache_destroy(kmem_cache_t* cachep)
{
	if (!cachep || cachep== cache_caches) return;
	wait(cachep->mutex);

	if (cachep->partial || cachep->full)
	{
		// kes nije moguce obrisati jer ima objekte koji se i dalje koriste
		cachep->errorCode = 5;
		signal(cachep->mutex);
		return;
	}

	//isprazni kes
	free_cache(cachep);

	for (int i = 0; i < NUMBER_OF_SMALL_CASHES; i++) {
		kmem_cache_t* cache = &small_cashes[i];
		if (cache == cachep) {
			//ne brise se kes koji je jedan od sizeN keseva
			cachep->errorCode = 4;
			signal(cachep->mutex);
			return;
		}
	}

	signal(cachep->mutex);


	//prevezivanje iz liste keseva
	kmem_cache_t* prev = cachep->prev;
	if (prev) {
		prev->next = cachep->next;
	}
	else {
		last_used = last_used->next;
	}

	if (cachep->next) {
		cachep->next->prev = prev;
	}

	//kes treba obrisati
	kmem_cache_free(cache_caches, cachep);


}

void kmem_cache_info(kmem_cache_t* cachep)
{ 
	if (!cachep)
		return;
	wait(cachep->mutex);
	int cache_size = sizeof(cachep)+cachep->number_of_slabs* (sizeof(Slab) + (sizeof(int) + cachep->object_size) * cachep->num_objects);

	int brBlck = prepare_number_of_Blocks(cache_size);

	double percentage_in_use = 0;
	if (cachep->number_of_slabs != 0) {
		percentage_in_use = 1.0 * (cachep->num_objects * cachep->number_of_slabs) / objects_in_use(cachep);
	}
	printf("----------------------------------------------------------");
	printf("Ime: ");
	printf(cachep->cache_name);
	printf("\n");
	printf("Velicina jednog podatka u bajtovima: %d\n", cachep->object_size);
	printf("Velicina celog kesa izrazenog u blokovima: %d\n", brBlck);
	printf("Broj ploca: %d\n", cachep->number_of_slabs);
	printf("Broj objekata u jednoj ploci: %d\n", cachep->num_objects);
	printf("Procentualna popunjenost: %lf %\n", percentage_in_use);
	
	signal(cachep->mutex);
}

int kmem_cache_error(kmem_cache_t* cachep)
{
	wait(cachep->mutex);
	switch (cachep->errorCode)
	{
	case 0: {
		signal(cachep->mutex);
		return 0;
	}
	case 1: {
		printf("Nema mesta za alokaciju jednog objekta\n");
		signal(cachep->mutex);
		return -1;
	}
	case 2: {
		printf("Pokusano je da se oslobodi objekat koji je vec oslobodjen\n");
		signal(cachep->mutex);
		return -2;
	}
	case 3: {
		printf("Objekat se ne nalazi u datom kesu\n");
		signal(cachep->mutex);
		return -3;
	}
	case 4: {
		printf("Nije moguce obrisati jedan od sizeN keseva\n");
		signal(cachep->mutex);
		return -4;
	}
	case 5: {
		printf("Kes nije moguce obrisati zato sto jos uvek ima objekte koji se koriste\n");
		signal(cachep->mutex);
		return -5;
	}
	default:
		break;
	}
	signal(cachep->mutex);

}


