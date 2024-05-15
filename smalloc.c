#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "smalloc.h"
#include <string.h>
#include <sys/mman.h>
#include <assert.h>

smheader_ptr smlist = 0x0;

void *smalloc(size_t s) {
    smheader_ptr curr = smlist;
    while (curr != NULL) {
        if (!curr->used && curr->size >= s) {
            if (curr->size >= s + sizeof(smheader) + 24) {
                size_t remaining_size = curr->size - s - sizeof(smheader);
                smheader_ptr new_header = (smheader_ptr)((char *)curr + sizeof(smheader) + s);
                new_header->size = remaining_size;
                new_header->used = 0;
                new_header->next = curr->next;

                curr->size = s;
                curr->next = new_header;
            }
            curr->used = 1;
            return ((void *)curr) + sizeof(smheader);
        }
        curr = curr->next;
    }

    size_t alloc_size = s + sizeof(smheader);
    size_t pages_needed = (alloc_size + getpagesize() - 1) / getpagesize();
    size_t total_alloc_size = pages_needed * getpagesize();
    void *allocated_block = mmap(NULL, total_alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (allocated_block == MAP_FAILED) return NULL;

    smheader_ptr new_block = (smheader_ptr)allocated_block;
    new_block->size = total_alloc_size - sizeof(smheader);
    new_block->used = 1;
    new_block->next = NULL;

    if (smlist == NULL) {
        smlist = new_block;
    } else {
        curr = smlist;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = new_block;
    }

    if (new_block->size >= s + sizeof(smheader) + 24) {
        size_t remaining_size = new_block->size - s - sizeof(smheader);
        smheader_ptr new_header = (smheader_ptr)((char *)new_block + sizeof(smheader) + s);
        new_header->size = remaining_size;
        new_header->used = 0;
        new_header->next = NULL;

        new_block->size = s;
        new_block->next = new_header;
    }

    return ((void *)new_block) + sizeof(smheader);
}

void *smalloc_mode(size_t s, smmode m) {
    if (s == 0) {
        return NULL;
    }

    smheader_ptr prev = NULL;
    smheader_ptr itr = smlist;

    switch (m) {
        case worstfit: {
            smheader_ptr worst_fit_block = NULL;
            size_t worst_fit_size = 0;

            while (itr != NULL) {
                if (!itr->used && itr->size >= s && itr->size > worst_fit_size) {
                    worst_fit_block = itr;
                    worst_fit_size = itr->size;
                }
                itr = itr->next;
            }

            if (worst_fit_block != NULL) {
                if (worst_fit_block->size >= s + sizeof(smheader) + 24) {
                    smheader_ptr new_header = (smheader_ptr)((char*)worst_fit_block + sizeof(smheader) + s);
                    new_header->size = worst_fit_block->size - s - sizeof(smheader);
                    new_header->used = 0;
                    new_header->next = worst_fit_block->next;
                    worst_fit_block->size = s;
                    worst_fit_block->next = new_header;
                }
                worst_fit_block->used = 1;
                return (void*)((char*)worst_fit_block + sizeof(smheader));
            }
            break;
        }
        case bestfit: {
            smheader_ptr best_fit_block = NULL;
            size_t best_fit_size = SIZE_MAX;

            while (itr != NULL) {
                if (!itr->used && itr->size >= s && itr->size < best_fit_size) {
                    best_fit_block = itr;
                    best_fit_size = itr->size;
                }
                itr = itr->next;
            }

            if (best_fit_block != NULL) {
                if (best_fit_block->size >= s + sizeof(smheader) + 24) {
                    smheader_ptr new_header = (smheader_ptr)((char*)best_fit_block + sizeof(smheader) + s);
                    new_header->size = best_fit_block->size - s - sizeof(smheader);
                    new_header->used = 0;
                    new_header->next = best_fit_block->next;
                    best_fit_block->size = s;
                    best_fit_block->next = new_header;
                }
                best_fit_block->used = 1;
                return (void*)((char*)best_fit_block + sizeof(smheader));
            }
            break;
        }
        case firstfit: {
            while (itr != NULL) {
                if (!itr->used && itr->size >= s) {
                    if (itr->size >= s + sizeof(smheader) + 24) {
                        smheader_ptr new_header = (smheader_ptr)((char*)itr + sizeof(smheader) + s);
                        new_header->size = itr->size - s - sizeof(smheader);
                        new_header->used = 0;
                        new_header->next = itr->next;
                        itr->size = s;
                        itr->next = new_header;
                    }
                    itr->used = 1;
                    return (void*)((char*)itr + sizeof(smheader));
                }
                itr = itr->next;
            }
            break;
        }
        default:
            return NULL; // Unknown allocation mode
    }

    // No suitable block found, allocate a new block
    size_t alloc_size = (s + sizeof(smheader) + getpagesize() - 1) & ~(getpagesize() - 1);
    smheader_ptr new_block = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new_block == MAP_FAILED) {
        return NULL;
    }
    new_block->size = alloc_size - sizeof(smheader);
    new_block->used = 1;
    new_block->next = NULL;

    if (smlist == NULL) {
        smlist = new_block;
    } else {
        itr = smlist;
        while (itr->next != NULL) {
            itr = itr->next;
        }
        itr->next = new_block;
    }
    return (void*)((char*)new_block + sizeof(smheader));
}



void sfree(void *p) {
    smheader_ptr curr = smlist;
    while (curr != NULL && ((char *)curr + sizeof(smheader)) != p) {
        curr = curr->next;
    }
    if (curr == NULL) {
        fprintf(stderr, "Invalid pointer: %p\n", p);
        abort();
    }
    curr->used = 0;
}

void *srealloc(void *p, size_t s) {
    if (p == NULL) {
        return smalloc(s);
    }
    if (s == 0) {
        sfree(p);
        return NULL;
    }

    smheader_ptr header = (smheader_ptr)((char *)p - sizeof(smheader));
    size_t original_size = header->size;

    void *new_p = smalloc(s);
    if (new_p != NULL) {
        memcpy(new_p, p, original_size < s ? original_size : s);
        sfree(p);
    }
    return new_p;
}

void smcoalesce() {
    smheader_ptr curr = smlist;

    while (curr != NULL && curr->next != NULL) {
        smheader_ptr next = curr->next;

        // Check if both blocks are unused and physically adjacent
        if (!curr->used && !next->used &&
            (char *)curr + sizeof(smheader) + curr->size == (char *)next) {
            // Merge the blocks
            curr->size += next->size + sizeof(smheader);
            curr->next = next->next;
        } else {
            curr = next; // Move to the next block
        }
    }
}


void smdump () 
{
	smheader_ptr itr ;

	printf("==================== used memory slots ====================\n") ;
	int i = 0 ;
	for (itr = smlist ; itr != 0x0 ; itr = itr->next) {
		if (itr->used == 0)
			continue ;

		printf("%3d:%p:%8d:", i, ((void *) itr) + sizeof(smheader), (int) itr->size) ;

		int j ;
		char * s = ((char *) itr) + sizeof(smheader) ;
		for (j = 0 ; j < (itr->size >= 8 ? 8 : itr->size) ; j++)  {
			printf("%02x ", s[j]) ;
		}
		printf("\n") ;
		i++ ;
	}
	printf("\n") ;

	printf("==================== unused memory slots ====================\n") ;
	i = 0 ;
	for (itr = smlist ; itr != 0x0 ; itr = itr->next, i++) {
		if (itr->used == 1)
			continue ;

		printf("%3d:%p:%8d:", i, ((void *) itr) + sizeof(smheader), (int) itr->size) ;

		int j ;
		char * s = ((char *) itr) + sizeof(smheader) ;
		for (j = 0 ; j < (itr->size >= 8 ? 8 : itr->size) ; j++) {
			printf("%02x ", s[j]) ;
		}
		printf("\n") ;
		i++ ;
	}
	printf("\n") ;
}
