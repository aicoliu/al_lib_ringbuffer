#include "ring_buffer.h"


/*** Private functions ***/
int rb_collate(struct ring_mm *, struct mem_block *);		/* Merges adjacent free blocks to form larger free blocks */
struct mem_block * rb_separate(struct ring_mm *, struct mem_block *, int);
struct mem_block * rm_get_nonmanifest_block(struct ring_mm *);
int rb_memcpy(struct ring_mm *, struct mem_block *, char *, int);
int rb_wrap(int);



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
			/* return 0; */
		}
		
		/* Skip over nonmanifested blocks */
		if (following_block->manifest == 0) {
			continue;
		}
		
		if (following_block->start_index == following_block_start_index) {
			/* At this point following_block == the actual folling block, now check if it's free */
			
			/* Cannot collate if following block in use */
			/* if (ring_buffer->mem_blocks[i].in_use == 1) { */
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

