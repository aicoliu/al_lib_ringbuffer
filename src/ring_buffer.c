#include "ring_buffer.h"

struct mem_block * rb_separate(struct ring_mm * ring_buffer, struct mem_block * block_to_separate, int length);

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
 * rb_get_nonmanifest_block - returns a nonmanifested block from the metablock list
 * 
 * input: ring buffer structure
 * output: struct mm_block object (with no manifestation in the buffer)
 *         returns NULL if there are non remaining
 */
struct mm_block * rb_get_nonmanifest_block(struct ring_mm * ring_buffer)
{
	int i;
	
	/* Select first non-manifested block */
	for (i=0; i < MAX_ITEMS; i++) {
		if (ring_buffer->mem_blocks[i].manifest == 0) {
			return (struct mm_block *)&(ring_buffer->mem_blocks[i]);
		}
	}
	
	return NULL;
}

/**
 * rb_separate - take a manifested but unused block and separate into two pieces,
 *               the first piece for the data, the second to remain as free
 * 
 * input: mem_block structure to be separated (written into)
 * output: mem_block stucture that remains free (as the user is expected to maintain the
 *         original pointer
 */
struct mem_block * rb_separate(struct ring_mm * ring_buffer, struct mem_block * block_to_separate, int length)
{
	struct mem_block * empty_block;
	int remainder_size;
	
	if (block_to_separate->in_use == 1) {
		return NULL;
	}
	
	if (block_to_separate->length < length) {
		return NULL;
	}
	
	empty_block = rb_get_nonmanifest_block(ring_buffer);
	
	if (empty_block == NULL) {
		return NULL;
	}
	
	/* Size of just-manifested block */
	remainder_size = block_to_separate->length - length;
	
	/* New block is not in use (free), but manifested in the ring buffer */
	empty_block->in_use = 0;
	empty_block->manifest = 1;
	
	/* Set start index and length of free block */
	empty_block->start_index = rb_wrap(block_to_separate->start_index + length);
	empty_block->length = remainder_size;

	
	/* Collate separated chunk */
	/* rb_collate(ring_buffer, empty_block); */
	
	/* Shorten the size of the original block */
	block_to_separate->length = length;

	
	/* Return block chopped off the end of the original */
	return empty_block;
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
 * rb_memcpy - copy memory to a ring buffer from an arbitrary addres, for a 
 *             certain number of bytes
 * 
 * WARNING: you can pass an arbitrary length parameter, but this function will only copy
 *          to the size specified in the destination block.  So, be sure to check that 
 *          the amount of bytes copied is equal to the length you passed to this function.
 * 
 * input: ring buffer (destination), start address of source, number of bytes
 * output: amount of bytes copied
 */
int rb_memcpy(struct ring_mm * ring_buffer, struct mem_block * dest_block, char * start_address, int length)
{
#ifdef NATIVE_MEMCPY

	int i, src_wrap_offset, remainder_length, bytes_copied = 0;
	
	
	if (ring_buffer == NULL) {
		return -1;
	}
	
	if (dest_block == NULL) {
		return -1;
	}
	
	if (start_address == NULL) {
		return -1;
	}
	
	/* Return error if destination block not large enough */
	if (dest_block->length < length) {
		return -1;
	}
	
	if (length <= 0) {
		return -1;
	}
	
	/* If this data will need to wrap around end of buffer */
	if (dest_block->start_index + dest_block->length > BUFFER_SIZE) {
		src_wrap_offset = BUFFER_SIZE - dest_block->start_index; /* - dest_block->length; */
		remainder_length = length - src_wrap_offset;

		/**
		 * WARNING:
		 * This could be a problem if not word-aligned...
		 * Though, I have no idea....
		 */

		/*
		printf(" *** [1] Copying from %16X (index=%d) for %d byte(s)...\n", start_address, 0, src_wrap_offset);
		*/
		
		/* NOTE:  memcpy(destination, source, length) */
		
		/* Copy to edge of buffer */
		memcpy(	ring_buffer->data + dest_block->start_index,
				start_address,
				src_wrap_offset);
		
		/*
		printf(" *** [2] Copying from %16X (index=%d) for %d byte(s)...\n", start_address, src_wrap_offset, remainder_length);
		*/
		
		/* Then wrap around and copy the remainder */		
		memcpy( ring_buffer->data,
				start_address + src_wrap_offset /* + 1 */,
				remainder_length);
				
		return length;		
	} else {
		
		/* If data does not need to be wrapped... */
		src_wrap_offset = remainder_length = 0;
		
		memcpy( ring_buffer->data + dest_block->start_index,
				start_address,
				length);
		return length;
	}
	

#else
	int i, bytes_copied = 0;
	
	/* Copy byte-by-byte (very slow!) */
	for (i=0; (i < length) && (i < dest_block->length); i++) {
		ring_buffer->data[rb_wrap(dest_block->start_index + i)] = start_address[i];
		bytes_copied++;
	}
	
	return bytes_copied;	
	
#endif /*NATIVE_MEMCPY*/
}


/**
 * rb_wrap - wraps a ring buffer index
 * 
 * input: unwrapped index
 * output: wrapped index (ensures index remains in buffer bounds)
 */
int rb_wrap(int unwrapped_index)
{
	return (unwrapped_index % BUFFER_SIZE);
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


/**
 * rb_collate - Merge the given block into the following block, if it is free
 * 
 * input: ring buffer structure, block to collate
 * output: 
 *         0 if cannot collate (ok), 
 *         1 if it merged with next block (ok and good!)
 *        -1 if block_to_collate isn't free (not good, but shouldn't cause problem)
 *        -2 if no following block (can indicate serious problem if working with non-empty ring buffer
 *        -3 if parameter is null
 */
int rb_collate(struct ring_mm * ring_buffer, struct mem_block * block_to_collate)
{
	struct mem_block * following_block;
	int i, following_block_start_index;
	
	if (block_to_collate == NULL) {
		return -3;
	}
	
	/* Return -1 (error).  Cannot collate block in use */
	if (block_to_collate->in_use == 1) {
		return -1;
	}
	
	if (block_to_collate->manifest == 0) {
		return -1;
	}
	
	following_block_start_index = rb_wrap(block_to_collate->start_index + block_to_collate->length);
	
	/* If following block index = block_to_collate start index, this probably means we have an empty buffer, and return -2 */
	if (following_block_start_index == block_to_collate->start_index) {
		return -2;
	}

	
	/* following_block_start_index = rb_wrap(block_to_collate->start_index + block_to_collate->length); */
	
	
	/* Find following block */
	for (i=0;i<MAX_ITEMS;i++) {
		/* At this point we are not sure if following_block == the actual block following block_to_collate */
		following_block = &(ring_buffer->mem_blocks[i]);
		
		/*
		//printf("  COLLATE: comparng block (index = %d) to metablock %d\n",block_to_collate->start_index, i);
		//printf("           wrap(index + length) = %d, metablock[%d].start = %d\n",
		//				following_block_start_index, i, following_block->start_index);		
		*/		

		/* Skip over itself */
		if (following_block == block_to_collate) {
			continue;
		}
		
		/* Cannot merge with block in use */
		if (following_block->in_use == 1) {
			continue;
		}
		
		/* Skip over nonmanifested blocks */
		if (following_block->manifest == 0) {
			continue;
		}
		
		if (following_block->start_index == following_block_start_index) {
			/* At this point following_block == the actual folling block, now check if it's free */
			
			/* Cannot collate if following block in use */
			if (following_block->in_use == 1) {
				return 0;
			}

			/* Merge block_to_collate with the following free block, creating a larger free block */
			block_to_collate->length += following_block->length;

			/* Following block is not longer in use */
			following_block->in_use = 0;
			
			/* ... and no longer manifested in the ring buffer */
			following_block->manifest = 0;
			
			block_to_collate->manifest = 1;
			
			/* Return status code that collate successful */
			return 1;
		}
	}

	/* -2 is status code indicating it cannot find a following block.
	 * This is a big problem (unless buffer consists of just one empty block). */
	return -2;
}

