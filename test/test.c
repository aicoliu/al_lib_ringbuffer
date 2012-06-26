

#include "../src/ring_buffer.h"


void print_buffer(struct ring_mm * ring_buffer)
{
	int i, j, k;
	int found = 0;
	
	for (i=0; i < BUFFER_SIZE; i++) {
		if (i != 0) {
			printf("|");
		}
		printf("%02X",(ring_buffer->data[i] & 0x000000FF));
	}
	
	printf("\n");
	
	if (BUFFER_SIZE < 80) {
		for (i=0; i < BUFFER_SIZE; i++) {
			found = 0;	
			for (j=0; j < MAX_ITEMS; j++) {
				if (ring_buffer->mem_blocks[j].manifest == 1
					&& ring_buffer->mem_blocks[j].start_index == i) {

					if (ring_buffer->mem_blocks[j].in_use == 0) {
						printf("*%02d", ring_buffer->mem_blocks[j].length);
					} else {
						printf("^%02d", ring_buffer->mem_blocks[j].length);
					}
					found = 1;
				}
				
			}
			if (found == 0) {
				printf("   ");
			}
		}
		printf("\n");
	}
}

int main(int argc, char ** argv)
{

	struct ring_mm ring_buffer;
	
	rb_init(&ring_buffer);
	
	print_buffer(&ring_buffer);
	
	rb_write(&ring_buffer, "xxxxxx", 6);
	
	print_buffer(&ring_buffer);
	
	rb_write(&ring_buffer, "000000000", 9);
	
	print_buffer(&ring_buffer);
	
	rb_free(&ring_buffer, 0);
	
	print_buffer(&ring_buffer);
	
	rb_write(&ring_buffer, "999999999", 9);
	
	print_buffer(&ring_buffer);
	
	rb_write(&ring_buffer, "aaaaaaaaa", 9);
	
	print_buffer(&ring_buffer);
	
	return 0;
}


