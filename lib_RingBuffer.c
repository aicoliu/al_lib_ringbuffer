#include <stdio.h>
#include <string.h>
#include "lib_RingBuffer.h"



/*
  初始化环形buffer
*/
void RB_init(struct RB_Buffer * buf)
{
	int i;
	for (i=0; i < RB_Max_Items; i++)
	{
		buf->items[i].read_index = 0;
		buf->items[i].length = 0;
	}
	
	buf->read_index = 0;
	buf->write_index = 0;

	buf->item_read_index=0;
	buf->item_write_index=0;
	buf->item_counts=0;

	buf->item_counts = 0;
	buf->length = 0;
	buf->size =RB_BUFFER_SIZE;
	buf->status = RB_Status_Free;
	
}

int RB_write(struct RB_Buffer * buf, const char * data, int length)
{
	 int FirstPart;
	 int SecondPart;

	 buf->status = RB_Status_Busy;
	
	 buf->items[buf->item_write_index].read_index =buf->write_index;
	 buf->items[buf->item_write_index].length=length;
	 buf->item_write_index++;
	 buf->item_counts++;
	 if (buf->item_write_index>RB_Max_Items-1) buf->item_write_index=0;

	 //printf("[%s]\n",data);

	 if ( ((buf->write_index)+length)  <  buf->size  )	//[本圈]	no wrap-around 
	 {
	   	memcpy(&buf->data[buf->write_index],data,length); //写入 buffer

		buf->write_index += length;  //标记write入位置
	 }
	 else  											// [换圈]	data wraps around
	 {

	    FirstPart=(buf->size) - (buf->write_index);
		SecondPart=length-FirstPart;
		memcpy(&buf->data[buf->write_index],data, FirstPart);
		memcpy(&buf->data[0],&data[FirstPart], SecondPart);
		buf->write_index =SecondPart;  //标记write入位置
		
	 }

	 buf->status = RB_Status_Free;
}
/*

  return read number,  0--Fail  >0 succ
*/

int RB_ReadItem(struct RB_Buffer * buf,  char *data, int SizeofData)
{
   int readcount=0;
   int readPos,len;
   int FirstPart,SecondPart;

   if (buf->item_counts<=0) return 0;

   readPos=buf->items[buf->item_read_index].read_index;

   len	  =buf->items[buf->item_read_index].length;

   if ( ( readPos+len ) <  buf->size  )	  //圈内数据
   {
   	   buf->read_index += len; 
	   if (len>SizeofData) len=SizeofData;  	   
       memcpy(data, &buf->data[readPos], len);   	   
			 
   }
   else   //跨圈 To Do: data空间不够，会出错 
   {
   	  FirstPart=buf->size - readPos;
	  SecondPart =len-FirstPart ;
      memcpy(data, &buf->data[readPos], FirstPart);
      memcpy(&data[FirstPart], &buf->data[0], SecondPart);
	  buf->read_index = SecondPart; 
   }



   buf->item_read_index++;	 //第一组数据索引+1
   if (buf->item_read_index>RB_Max_Items-1) buf->item_read_index=0;
   buf->item_counts--;

   return len; 

}


short int RB_GetItemsCount(struct RB_Buffer * buf)
{
	return buf->item_counts;
}

char * RB_GetAllData(struct RB_Buffer * buf)
{
	return (buf->data);
}


