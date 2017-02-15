#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <getopt.h>

int verbose = 0;

#define FBM_BB      0
#define FBM_LFB     1
#define FBM_INDX    2
#define FBM_FBT     3

typedef __int32_t block_id_t;     //from 0
typedef __int32_t file_size_t;    

//HarhDisk parament
#define HD_BLOCK_NR     500
#define HD_BLOCK_SIZE   (2*1024)

//FILE_HEAD
#define FILE_MAX_SIZE           (10*1024)
#define FILE_MIN_SIZE           (2*1024)
#define FILE_MAX_NODES          5
#define FILE_NAME_MAX_LENGTH    16

typedef struct FILE_HEAD{
    int file_node;                          //4
    file_size_t file_size;                  //4
    char filename[ FILE_NAME_MAX_LENGTH ];  //16
    block_id_t blocks[ FILE_MAX_NODES ];    //20
    block_id_t block_nr;                    //4
}FILE_HEAD;
#define FILE_HEAD_SIZE (sizeof(FILE_HEAD))  //48

//FBM_FBT
typedef struct _FreeBlockTable{
    block_id_t blocks[ HD_BLOCK_NR ];   //4 * 500 = 2000 B
    block_id_t length;                  //4 B
    block_id_t top;                     //4 B     //point at the topest block id's next
    block_id_t bottom;                  //4 B     //point at the bottom block id
}_FreeBlockTable;                       //2012 B  //store in HD_BLOCK[ 0 ]
static _FreeBlockTable HD_BLOCK_TABLE;

int getFreeHDBlock(){
    if( HD_BLOCK_TABLE.top == HD_BLOCK_TABLE.bottom ) return -1;
    int free_block_id = HD_BLOCK_TABLE.blocks[ HD_BLOCK_TABLE.bottom ];
    HD_BLOCK_TABLE.bottom = (HD_BLOCK_TABLE.bottom + 1 ) % HD_BLOCK_TABLE.length;
    return free_block_id;
}

int addFreeHDBlock(int block_id){
    if( (HD_BLOCK_TABLE.top + 1) % HD_BLOCK_TABLE.length == HD_BLOCK_TABLE.bottom ) return 1;

    int index = HD_BLOCK_TABLE.bottom;
    while(index != HD_BLOCK_TABLE.top){
        if( HD_BLOCK_TABLE.blocks[ index ] == block_id ) return 1;
        index = (index + 1) % HD_BLOCK_TABLE.length;
    }

    HD_BLOCK_TABLE.blocks[ HD_BLOCK_TABLE.top ] = block_id;
    HD_BLOCK_TABLE.top = (HD_BLOCK_TABLE.top + 1) % HD_BLOCK_TABLE.length;
}

void print_FreeBlockTable()
{
    printf("\nFREE BLOCK Table\n");
    printf("BLOCK[0] stores freeBlockTable\n");
    printf("Block[1]&[2] stores all file heads\n");
    printf("TOP: %5d BOTTOM: %5d\n",HD_BLOCK_TABLE.top,HD_BLOCK_TABLE.bottom);
    int index = HD_BLOCK_TABLE.bottom;
    int count = 0;
    while(index != HD_BLOCK_TABLE.top){
        printf("%5d ",HD_BLOCK_TABLE.blocks[index]);
        index = (index + 1) % HD_BLOCK_TABLE.length;
        ++count;
        if(count % 10 == 0) printf("\n");
    }
    printf("\n");
}

//FileSystem
#define FILE_MAX_NR 85
struct FileManageSystem{                //store in HD_BLOCK[ 1,2 ]
    FILE_HEAD* file_heads[ FILE_MAX_NR ];
    int file_nr;
}FM_SYS;

static int file_node_count = 0;  //get free file node
int getFreeFileNode(){
    //if(file_node_count == FILE_MAX_NR) return 0;
    return ++file_node_count;
}

int getFileHeadIndex(int file_node)
{
    for(int i=0; i<FM_SYS.file_nr; ++i){
        FILE_HEAD* head = FM_SYS.file_heads[ i ];
        if( head->file_node == file_node ) return i;
    }
    return -1;
}

FILE_HEAD * getFileHead(int file_node)
{//if fail then return NULL , if success the return FILE_HEAD*
    int index = getFileHeadIndex(file_node);
    if(index == -1) return NULL;
    else return FM_SYS.file_heads[ index ];
}

int creatFile(char *filename, file_size_t file_size)
{//if fail then return 0 ; if success then return file_node
    if( FM_SYS.file_nr == FILE_MAX_NR ) return 0;

    if(verbose){ printf("\ncreate file:%s\n",filename);
    printf("file size: %d\n",file_size);}

    int new_file_node = getFreeFileNode();
    if( new_file_node == 0 ) return 0;     //get new file node

    if(verbose) printf("generate file node:%d\n",new_file_node);

    int file_block_nr = file_size / HD_BLOCK_SIZE + (file_size % HD_BLOCK_SIZE == 0 ? 0 : 1 );
    if( file_block_nr > FILE_MAX_NODES ) return 0;  //

    if(verbose) printf("file block nr:%d\n",file_block_nr );

    FILE_HEAD* head = (FILE_HEAD*)malloc(FILE_HEAD_SIZE);//malloc new file_head
    head->file_node = new_file_node;
    strcpy( head->filename, filename );
    head->file_size = file_size;
    head->block_nr = file_block_nr;
    for(int i=0; i<head->block_nr; ++i){
        int hdblock = getFreeHDBlock(); //no more HD space
        if(verbose) printf("store in block: %d\n",hdblock);
        if( hdblock == -1 ){
            free(head);
            return 0;
        }
        head->blocks[i] = hdblock;
    }
    
    int new_file = FM_SYS.file_nr;  //registe in file system
    FM_SYS.file_heads[ new_file ] = head;
    FM_SYS.file_nr += 1;
    
    return new_file_node;   //return file node
}

int freeFile(int f)
{//if fail then return 0; if success then return 1;
    int index = getFileHeadIndex(f);
    if(index == -1) return 0;

    FILE_HEAD* head = FM_SYS.file_heads[index];
    if( head == NULL ) return 0;

    if(verbose) {printf("\ndelete file: %s\n",head->filename);}

    for(int i=0; i<head->block_nr; ++i){
        addFreeHDBlock( head->blocks[i] );
    }

    for(int i=index; i<FM_SYS.file_nr - 1; ++i){
        FM_SYS.file_heads[i] = FM_SYS.file_heads[i+1];
    }
    FM_SYS.file_nr -= 1;    

    free(head);
    
    return 1;
}

void init()
{
    FM_SYS.file_nr = 0;  //init FileManageSystem
    HD_BLOCK_TABLE.length = HD_BLOCK_NR;
    for(int i=3; i<HD_BLOCK_NR; ++i){//[0] stores FreeBlockTable [1]&[2] stores file_heads
        addFreeHDBlock(i);//add free block id
    }
}

int _RandomFileNr = 50;
void RandomFileProcess()
{
    int randomFile[ _RandomFileNr ];
    char randomFileName[ FILE_NAME_MAX_LENGTH ];

    srand((unsigned int)time(NULL));
    for(int i=0; i<_RandomFileNr; ++i){    //create
        file_size_t file_size = rand()% ( FILE_MAX_SIZE - FILE_MIN_SIZE ) + FILE_MIN_SIZE;
        sprintf(randomFileName,"%d.txt",i+1);
        randomFile[ i ] = creatFile( randomFileName , file_size );
    }
    
    for(int i=0; i<_RandomFileNr; i+=2){   //delete
        freeFile( randomFile[ i ] );
    }
}

void printFileHead(int f)
{
    FILE_HEAD* head = getFileHead(f);
    if( head == NULL ) return ;

    printf("\n");
    printf("file node:       %d\n",head->file_node );
    printf("file name:       %s\n",head->filename );
    printf("file size:       %11d\n",head->file_size );
    printf("file block_nr:   %11d\n",head->block_nr );
    for(int i=0; i<head->block_nr; ++i){
        printf("file block[%2d]  %11d\n",i,head->blocks[i] );
    }
}

void run()
{
    RandomFileProcess();
    if(verbose) print_FreeBlockTable();

    int F1,F2,F3,F4,F5;

    F1 = creatFile("F1.txt",7*1024 );        //7 K
    F2 = creatFile("F2.txt",5*1024 );        //5 K
    F3 = creatFile("F3.txt",2*1024 );        //2 K
    F4 = creatFile("F4.txt",9*1024 );        //9 K
    F5 = creatFile("F5.txt",3*1024+512 );    //3.5 K
    
    if(verbose) print_FreeBlockTable();

    printFileHead(F1);
    printFileHead(F2);
    printFileHead(F3);
    printFileHead(F4);
    printFileHead(F5);
}

int main(int argc,char*argv[])
{
	char c = 0;

	struct option longopts[]=
	{
		//{"managefreeblock",1,0,'f'},
        {"verbose",0,0,'v'},
		{0,0,0,0}
	};

	while((c=getopt_long(argc,argv,"f:v",longopts,NULL))!=EOF){
		switch(c)
		{
		case 'v':
			verbose = 1;
			break;
		//case 'f':
		//	FreeBlocksManage = atoi(optarg);
		//	break;
		default:
			printf("Wrong Option!\n");
			goto _MAIN_OVER;
			break;
		}
	}

    printf("---- HARDWARE INFO --\n");
    printf("Physical Block Nr       %11d\n",HD_BLOCK_NR);
    printf("Physical Block SIZE     %11d B\n",HD_BLOCK_SIZE);
    printf("---- FILE SYSTEM ----\n");
    printf("File Max Size:          %11d B\n",FILE_MAX_SIZE);
    printf("File Min Size:          %11d B\n",FILE_MIN_SIZE);
    printf("File Node Size:         %11lu B\n",FILE_HEAD_SIZE);
    printf("File Max Name length:   %11d\n",FILE_NAME_MAX_LENGTH);
    printf("File Max Nr:            %11d\n",FILE_MAX_NR);

    init();
    if(verbose) print_FreeBlockTable();

    run();

_MAIN_OVER:
	return 0;
}