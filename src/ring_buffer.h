#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

#define BUFFER_SIZE	30 /* (2048) */
#define MAX_ITEMS	(25)
#define SWAP_SPACE (2048)

#define NULL ((void *)0)



/**
 * TODO: Formalize error codes ...
 * 
 * 
 * 
 */



/*#define USING_TIME	1*/
#define NATIVE_MEMCPY	1


#ifdef NATIVE_MEMCPY
#include <string.h>
#endif


/**
 * mem_block - Memory Block
 *  This structure is a metablock for a chunk of data in a ring buffer
 */
struct mem_block {
	int manifest;		/* 0 if this buffer has no manifestation in ring buffer (ie: not yet used)*/
	int in_use;		/* 0 if this buffer is free, 1 if the data in the block is used (usually containing packet) */
	int start_index;	/* start index of this block of data in the buffer */
	int timestamp;		/* time this data was entered into buffer, used for establishing priority */
	int length;		/* length of data in the buffer */
};


/**
 * ring_mm -  Ring Memory Manager
 *   Used in the dynamic allocation of data chunks to a ring buffer
 */
struct ring_mm {
	char data[BUFFER_SIZE];	/* The buffer where everything is stored. */
	struct mem_block mem_blocks	[MAX_ITEMS];	/* Each block in the buffer has a metadata structure in this array. */
	char swap 			[SWAP_SPACE];	/* Space for temporary buffering and swaps */
	int swap_in_use;							/* Set to 1 if swap space is in use */
};

/** FIX ALL THIS WHEN THE C FILE IS DONE **/

/*** Outward facing functions ***/
void rb_init(struct ring_mm *);
int rb_write(struct ring_mm *, const char *, int); /* source address of data, length to copy */
int rb_read(const struct ring_mm *, int, const char *, int); /* destination address to copy to, start index in buffer containing data */
int rb_free(struct ring_mm*, int); /* index of start of block to free */
void rb_status(const struct ring_mm *, const char *);


/*** Private functions ***/
#if 0
//int rb_collate(struct ring_mm *, struct mem_block *);		/* Merges adjacent free blocks to form larger free blocks */
//struct mem_block * rb_separate(struct ring_mm *, struct mem_block *, int);
//struct mem_block * rm_get_nonmanifest_block(struct ring_mm *);
//int rb_memcpy(struct ring_mm *, struct mem_block *, char *, int);
//int rb_wrap(int);
#endif

#endif

