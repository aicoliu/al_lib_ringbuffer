
#define RB_BUFFER_SIZE  128  //(4*1024)  /*数据空间大小*/
#define RB_Max_Items    4        /*最多可以保存多少条记录*/
#define RB_Status_Free  0
#define RB_Status_Busy  1
/**
	用于记录整个内存片区的有效数据位置,
**/

struct RB_Buffer_Block
{
		int read_index;  /*数据开始位置*/
		int length;		  /*数据长度*/
};

struct RB_Buffer {
		char 	data[RB_BUFFER_SIZE]; 
		struct 	RB_Buffer_Block	items [RB_Max_Items];

		int 	read_index;  /*数据开始位置*/
		int 	write_index;	  /*数据结束位置*/
		int 	length;		  /*所有数据长度*/	
		char 	status;		  /*操作状态*/
		int     size;		  /*仓库总大小*/

  short int     item_read_index;	   //第一组数据的index
  short int     item_write_index;	   //最后一组数据的idx
  short int     item_counts;

};

/*环形buffer初始化*/
void  RB_init(struct RB_Buffer * buf);

/*数据产生函数->数据加入队列*/
int   RB_write(struct RB_Buffer * buf, const char * data, int length);

/*数据消耗函数->取出队列First in数据*/
int   RB_ReadItem(struct RB_Buffer * buf,  char *data, int SizeofData);	

/*队列中数据个数*/
short int RB_GetItemsCount(struct RB_Buffer * buf);

char * RB_GetAllData(struct RB_Buffer * buf);

/**
//例子：
struct RB_Buffer RBB;
char swap[32]
RB_init(&RBB);
RB_write(&RBB,"0123456789",10);
RB_ReadItem(&RBB,swap,32);
//RB_GetItemsCount(&RBB);

**/

