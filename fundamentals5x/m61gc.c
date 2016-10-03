#include "m61gc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


// In this exercise, we track allocations in an array sorted by allocated
// pointer (not a linked list). The sorted array lets us quickly find the
// allocation containing a pointer, using binary search.

// structure representing a single allocation
#define GET_ARRAY_LEN(array,len) {len = (sizeof(array) / sizeof(array[0]) );}

typedef struct allocation {
    char* ptr;                        // pointer to first allocated byte
    size_t sz;                        // size of allocation
    int marked;                       // used in GC
} allocation;

static allocation* allocs = NULL;     // active allocations (sorted by ptr)
static size_t nallocs = 0;            // number of active allocations
static size_t allocs_capacity = 0;    // capacity of `allocs` array

char* m61_stack_bottom;


static void allocation_atexit(void) {
    free(allocs);
}

static size_t find_allocation_index(char* ptr) {
    size_t l = 0, r = nallocs;
    while (l < r) {
        size_t m = l + (r - l) / 2;
        if (ptr < allocs[m].ptr)
            r = m;
        else if (ptr >= allocs[m].ptr + allocs[m].sz)
            l = m + 1;
        else
            return m;
    }
    return l;
}

static allocation* find_allocation(char* ptr) __attribute__((used));
static allocation* find_allocation(char* ptr) {
    size_t i = find_allocation_index(ptr);
    assert(i == nallocs || ptr < allocs[i].ptr + allocs[i].sz);
    if (i < nallocs && ptr >= allocs[i].ptr)
        return &allocs[i];
    else
        return NULL;
}

void* m61_malloc(size_t sz) {
    static size_t allocation_count = 0;
    ++allocation_count;

    void* ptr = malloc(sz);
    // Garbage collect every 2**16 allocations, or sooner if malloc fails
    if (!ptr || allocation_count % (1U << 16) == 0) {
        m61_gc();
        ptr = malloc(sz);
    }
    if (!ptr)
        return NULL;

    // find index to insert this allocation
    size_t i = find_allocation_index(ptr);
    // this ptr must not overlap with a current allocation
    assert(i == nallocs || (char*) ptr + sz <= allocs[i].ptr);
    // make space for the new allocation
    if (nallocs == allocs_capacity) {
        if (!allocs_capacity) {
            atexit(allocation_atexit); // free allocations array on exit
            allocs_capacity = 1024;
        } else
            allocs_capacity *= 2;
        allocs = realloc(allocs, sizeof(allocation) * allocs_capacity);
        assert(allocs);
    }
    memmove(&allocs[i + 1], &allocs[i], sizeof(allocation) * (nallocs - i));
    // store the new allocation
    allocs[i].ptr = ptr;
    allocs[i].sz = sz;
    ++nallocs;
    // clear and return it
    memset(ptr, 0, sz);
    return ptr;
}

void m61_free(void* ptr) {
	    if (!ptr)
	        return;
	    // find index of this allocation
	    size_t i = find_allocation_index(ptr);
	    // that allocation must match `ptr` exactly
	    if(i>=nallocs)
	    {
	    	printf("the index is %u, nallocs is %u  and the address is %p",i,nallocs,ptr);
	    }
	    assert(i <nallocs);
	    assert(allocs[i].ptr == ptr);
	    // remove the allocation from the list and free `ptr`
	    memmove(&allocs[i], &allocs[i + 1], sizeof(allocation) * (nallocs - i - 1));
	    --nallocs;
	    free(ptr);

}
void m61_print_allocations(void) {
    printf("%zu allocations\n", nallocs);
    for (size_t i = 0; i != nallocs; ++i)
        printf("  #%zu: %p: %zu bytes\n", i, allocs[i].ptr, allocs[i].sz);
}
static unsigned n_marked=0;
static void mark_allocations(const char* base, size_t sz) {
    (void) base;
    if (sz < sizeof(void*))
        return;
    n_marked++;
    for (size_t i = 0; i <= sz - sizeof(void*); ++i) {
    	char *ptr;
    	memcpy(&ptr,base+i,sizeof(ptr));
    	allocation * ptr_allocs=find_allocation(ptr);
    	if(ptr_allocs&&ptr_allocs->marked==0)
    	{
    		ptr_allocs->marked=1;
    		mark_allocations(ptr_allocs->ptr,ptr_allocs->sz);
    	}
        // check if the data at `base + i` contains a pointer
        // YOUR CODE HERE

    }
}

void m61_gc(void) {

#if __x86_64__
    // ensure all of our callers' variables are located on the stack,
    // rather than in registers
    __asm__ __volatile__("" : : : "rbx", "r12", "r13", "r14", "r15", "memory");
#endif

    char* stack_top = (char*) &stack_top;

    // unmark all active allocations
    // YOUR CODE HEREo
	for(size_t i=0;i!=nallocs;i++)
	{
		allocs[i].marked=0;

	}

    // mark allocations in the stack
    mark_allocations(stack_top, m61_stack_bottom - stack_top);

#if __linux__
    // mark allocations in globals
    extern char data_start[];
    extern char _end[];
    mark_allocations(data_start, _end - data_start);
#endif
    for(size_t i=0;i!=nallocs;i++)
    	{
			if(!allocs[i].marked)
			{
				size_t index=find_allocation_index(allocs[i].ptr);
				if(index!=i)
				{
					printf("i is %u,index is %u",i,index);
					assert(index==i);

				}

				m61_free(allocs[i].ptr);
			}

    	}
    // free unmarked allocations
    // YOUR CODE HERE


}


////////////////////////////////////////////////////////////////////////////////
/// Helper method definitions


