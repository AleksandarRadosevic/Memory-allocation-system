#pragma once
#include"slab.h"
#include"Utility.h"
#include"buddyAlocator.h"
#define NUMBER_OF_SMALL_CASHES 13

typedef struct slab {
	void* start_address;
	void* end_address;

	unsigned int offset;		//pomeraj u odnosu na slab
	unsigned int free;			//index prvog slobodnog 
	unsigned int in_Use;		//broj objekata koji se koristi	
	unsigned int max_objects;	//max objekata u slabu

	struct slab* next;
	struct slab* prev;


	unsigned int* array_Free;	//objekti koji se koristi

	kmem_cache_t* cache;		
}Slab;

typedef struct kmem_cache_s {
	
	char cache_name[80];			//ime kesa
	size_t object_size;				//velicina objekta
	unsigned int num_objects;		//broj objekata u ploci
	unsigned int offset;			//offset u odnosu na pocetak
	unsigned int number_of_slabs;
	unsigned int fragmentation;
	unsigned int errorCode;

	struct slab* empty;
	struct slab* full;
	struct slab* partial;

	kmem_cache_t* next;
	kmem_cache_t* prev;

	void(*ctor)(void*);	// konstruktor
	void(*dtor)(void*);	// destruktor
	HANDLE mutex;


}kmem_cache_t;


void init_cache(kmem_cache_t *c,char *name,size_t object_size, void(*ctor)(void*), void(*dtor)(void*));
void* get_free_object(Slab* s);
Slab* create_new_Slab(kmem_cache_t *cache);
void change_cache_offset(kmem_cache_t* s);
void Remove_Element_fromList_toAnother(Slab* first, Slab* second, Slab* element);
void free_object(kmem_cache_t* cache, Slab* slab, void* obj);
Slab* find_slab(kmem_cache_t* cache, void* obj);
int free_cache(kmem_cache_t* cache);
double objects_in_use(kmem_cache_t* cache);