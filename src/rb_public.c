#include "ring_buffer.h"


/*** Private functions ***/
int rb_collate(struct ring_mm *, struct mem_block *);		/* Merges adjacent free blocks to form larger free blocks */
struct mem_block * rb_separate(struct ring_mm *, struct mem_block *, int);
struct mem_block * rm_get_nonmanifest_block(struct ring_mm *);
int rb_memcpy(struct ring_mm *, struct mem_block *, char *, int);
int rb_wrap(int);



/**
 * rb_init - Initialize a ring buffer
 * 
 * input: a ring buffer struct to be initialized
 * output: none
 */
void rb_init(struct ring_mm * ring_buffer)
{
	int i;

	/* Initialize each metablock buffer */
	for (i=0; i < MAX_ITEMS; i++)
	{
		ring_buffer->mem_blocks[i].manifest = 0;
		ring_buffer->mem_blocks[i].in_use = 0;
		ring_buffer->mem_blocks[i].start_index = 0;
		ring_buffer->mem_blocks[i].timestamp = 0;
		ring_buffer->mem_blocks[i].length = 0;
	}
	
	/* Create one large free block the size of entire buffer */
	ring_buffer->mem_blocks[0].manifest = 1;
	ring_buffer->mem_blocks[0].start_index = 0;
	ring_buffer->mem_blocks[0].length = BUFFER_SIZE;
	
	ring_buffer->swap_in_use = 0;
}


/**
 * rb_status - Print statistics to an output buffer
 * 
 * input: ring buffer, output buffer to print status
 * output: none (void)
 */
void rb_status(const struct ring_mm * ring_buffer, const char * out_buffer)
{
	struct mem_block * curr;
	int bytes_allocated = 0;
	int total_bytes = 0;
	int manifested_blocks = 0;
	int blocks_in_use = 0;
	int free_blocks = 0;
	int i = 0;
	
	for (i=0;i<MAX_ITEMS;i++) {
		curr = &(ring_buffer->mem_blocks[i]);
		
		if (curr->manifest == 1) {
			manifested_blocks++;
			total_bytes += curr->length;


			if (curr->in_use == 1) {
				bytes_allocated += curr->length;
				blocks_in_use++;
			} else {
				free_blocks++;
			}

		}
	}
	
	sprintf(out_buffer, "Ring Buffer Stats: \n"
			"   metablocks:        %d\n"
			"   manifested blocks: %d\n"
			"     -blocks in use:  %d\n"
			"     -blocks free:    %d\n"
			"   accessable bytes:  %d\n"
			"   allocated bytes:   %d\n\n",
			MAX_ITEMS, manifested_blocks, blocks_in_use, free_blocks, total_bytes, bytes_allocated);
}

/**
 * rb_write - write a block of data to the buffer
 * 
 * input: ring structure, start address of memory to copy, length to copy
 * output: 0 on success, -ERRORVAL on error
 *         -1 not enough memory blocks left in memory manager OR none with enough room
 */
int rb_write(struct ring_mm * ring_buffer, const char * start_address, int length)
{
	struct mem_block * current_block;
	struct mem_block * open_block;
	int i, n, bytes_copied = 0;
	
	
	
	/* XXX
	// do something here like if( number of free blocks <= 2), return error
	*/
	
	
	/* Find first block with enough size (naive algorithm) */
	for (i=0; i < MAX_ITEMS; i++) {
		current_block = &(ring_buffer->mem_blocks[i]);
		
		/*
		//printf("  current->manifest = %d\n", current_block->manifest);
		//printf("  current->in_use   = %d\n", current_block->in_use);
		//printf("  current->length   = %d\n", current_block->length);
		*/		

		/* Find free block with sufficient size */
		if (current_block->manifest == 1 
			&& current_block->in_use == 0
			&& current_block->length >= length){
			
						
			/* If any leftover room at the end, turn it into a new free block
			 * or merge with adjacent free block */
			if (length < current_block->length) {
				/* If there are not enough leftover blocks, return error */
				if ((open_block=rb_separate(ring_buffer, current_block,length)) == NULL) {
					return -1;
				}
			}						
						
						
			/* Copy data to appropriate block in ring buffer */
			/* for (n = 0; n < length; n++) { */
				bytes_copied = rb_memcpy(ring_buffer, current_block,start_address, length); 

				if (bytes_copied != length) {
					return -2;
				}
			/* } */
			
			/* Set this block to being used */
			current_block->in_use = 1;
			
			
			rb_collate(ring_buffer,open_block);
			
			/* Now exit */
			return current_block->start_index;
		}
	}
	
	return -1;
}

/**
 * rb_read - (for now) print out contents of block
 * 
 * input: ring buffer, block in buffer to print
 * output: 0 if ok
 *         -1 if block not in use or not manifested
 */
int rb_read(const struct ring_mm * ring_buffer, int start_index, const char * dest, int length)
{
	int i, n, found = 0, length_to_copy, wrap_index;
	struct mem_block * block_to_read = NULL;
	void * memcpy_status;
	
	for (n=0;n<MAX_ITEMS;n++) {
		block_to_read = &(ring_buffer->mem_blocks[n]);
		
		if (block_to_read->start_index == rb_wrap(start_index)) {
			found = 1;
			break;
		}
	}
	
	if (found == 0) {
		 return -1;
	}
	
	if (block_to_read->in_use == 0) {
		return -1;
	}
	
	if (block_to_read->manifest == 0) {
		return -1;
	}
	
	length_to_copy = block_to_read->length;
	
	if (length < length_to_copy) {
		length_to_copy = length;
	}
	
	if (block_to_read->start_index + block_to_read->length >= BUFFER_SIZE) {
		wrap_index = BUFFER_SIZE - block_to_read->start_index;
	} else {
		wrap_index = 0;
	}
	
#ifdef NATIVE_MEMCPY
	if (wrap_index != 0) { /* + 1 */
		memcpy_status = memcpy(dest, ring_buffer->data + start_index, length_to_copy);
		if (memcpy_status != NULL) {
			return length_to_copy;
		}
		return 0;
	} else {
		memcpy(dest,ring_buffer->data + block_to_read->start_index, wrap_index);
		memcpy(dest + wrap_index,ring_buffer->data, length_to_copy - wrap_index);
		return length_to_copy;
	}
#else
	if (wrap_index != 0) { /* + 1 */
		for (i=0; i < length_to_copy; i++) {
			dest[i] = ring_buffer->data[block_to_read->start_index + i];
		}
	} else {
		/* Copy to end of buffer */
		for (i=0; i < wrap_index; i++) {
			dest[i] = ring_buffer->data[block_to_read->start_index + i];
		}
		
		/* Then wrap around and copy front of buffer */
		for (i=0; i<(length_to_copy-wrap_index); i++) {
			dest[wrap_index + i] = ring_buffer->data[i];
		}
	}
	
	return length_to_copy;
#endif
}


/**
 * rb_free - free a block of memory that begins at a certain index
 *           supposed to be identical to stdlib free, where you pass
 *           a pointer.
 * 
 * intput: ring buffer, index where data block starts
 * output: 0 if free successful
 *         -1 error (no block found with this start address)
 */
int rb_free(struct ring_mm * ring_buffer, int start_index)
{
	struct mem_block * current_block;
	int i, return_value;
	
	for (i=0; i < MAX_ITEMS; i++) {
		current_block = &(ring_buffer->mem_blocks[i]);
		
		if (current_block->start_index == start_index
			&& current_block->in_use == 1) {
			current_block->in_use = 0;
			
			return_value = 0;
			break;
		}
		return_value = 1;
	}
	
	if (return_value == 0) {
		/* This could be ugly/inefficient, but go through and try to collate all free blocks */
		for (i=0;i<MAX_ITEMS;i++) {
			current_block = &(ring_buffer->mem_blocks[i]);
			rb_collate(ring_buffer, current_block);
		}
	}
	
	return return_value;
}

