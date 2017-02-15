#include <stdio.h>
#include <stdlib.h>
#include <string.h>		//string
#include <sys/types.h>	//standard types
#include <getopt.h>		//getopt()
#include <unistd.h>
#include <fcntl.h>		//system call for file control : read open
#include <sys/stat.h>	//file information structure : struct stat
#include <time.h>		//time()
#include <sys/ipc.h>
#include <sys/sem.h>	//semphera
#include <sys/mman.h>	//mmap()	fail return MAP_FAILED ((void*)-1)

//system page size : sysconf(_SC_PAGESIZE)

int verbose = 0;

int sem_wsem,sem_x;
//int readcount = 0;
int *readcount;
key_t key_wsem = 555,key_x=556;

struct sembuf sem_wait;
struct sembuf sem_signal;

union semun{
	int val;
	struct semid_ds * buf;
	unsigned int * array;
#if defined(__linux__)
	struct seminfo * __buf;
#endif
};

char *mm_buffer;
char *mm_filename;
int mm_buffer_size;

void reader()
{
    int index = 0;
    while(index<mm_buffer_size){
        
        semop(sem_x, &sem_wait,1);
        (*readcount)++;
        //printf("readcount %d\n",*readcount);
        if(*readcount==1) semop(sem_wsem, &sem_wait,1);
        //printf("read begin\n");
        semop(sem_x, &sem_signal,1);

        printf("[%6d] read %c\n",getpid(),mm_buffer[index++]);

        semop(sem_x, &sem_wait,1);
        (*readcount)--;
        //printf("readcount %d\n",*readcount);
        if(*readcount==0) semop(sem_wsem, &sem_signal,1);
        //printf("read end\n");
        semop(sem_x, &sem_signal,1);

    }
}

void writer()
{
    int index = 0;
    while(index<mm_buffer_size){
        semop(sem_wsem,&sem_wait,1);	//enter
        mm_buffer[index] = 'a'+index;;
        printf("[%6d] write: %c\n",getpid(),mm_buffer[index]);
        index += 1;
        semop(sem_wsem,&sem_signal,1);	//retne
    }
}

int parbegin()
{
	int ret = 0;

	int sem_wsem_val = semctl(sem_wsem,0,GETVAL);
	int sem_x_val = semctl(sem_x,0,GETVAL);
	printf("sem_wsem = %d\n",sem_wsem_val);
	printf("sem_x = %d\n",sem_x_val);

	int pid = fork();
	if(pid == 0){
		//father
		//writer
        //int pid1 = fork();
        fork();
		writer();

        //if( pid1==0 ){
		//    semctl(sem_wsem,0,IPC_RMID);
		//    semctl(sem_x,0,IPC_RMID);
        //}
	}
	else if(pid > 0){
		//child
		//reader
        fork();fork();

		reader();
	}
	else{//pid < 0
		ret = 3;
		printf("Error : fork fail in parbegin()\n");
	}

	return ret;
}

void run()
{
	printf("constructing\n");
	for(int i=0;i<mm_buffer_size;++i){
		mm_buffer[i] = 'A'+i;
		printf("[%2d][%2d][%c]\n",i,mm_buffer[i],mm_buffer[i]);
	}

	int ret = parbegin();
	if(ret == 1){
		//father failed
		printf("Error : parbegin in run() in father process\n");
	}
	else if(ret == 2){
		//child failed
		printf("Error : parbegin in run() in child process\n");
	}
}

int init()
{
	int ret = 0;
	
	int fd = open(mm_filename,O_RDWR);
	if(fd==-1){
		ret = 1;
		printf("Open mm_file %s in init()\n",mm_filename);
		goto _INIT_OUT;
	}

	struct stat sb;
	if(fstat(fd,&sb)==-1){
		ret = 2;
		printf("fstat mm_file %s in init()\n",mm_filename);
		goto _INIT_OUT;
	}

	mm_buffer_size = sb.st_size;
	mm_buffer = mmap(NULL,sb.st_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if(mm_buffer == MAP_FAILED){
		ret = 3;
		printf("mmap mm_file %s in init()\n",mm_filename);
		goto _INIT_OUT;
	}

    readcount = mmap(NULL,4,PROT_READ|PROT_WRITE,MAP_ANON|MAP_SHARED,-1,0);
    *readcount = 0;

	union semun arg;

	sem_wsem = semget(key_wsem,1,IPC_CREAT|0666);
	arg.val = 1;
	semctl(sem_wsem, 0, SETVAL, arg);

	sem_x = semget(key_x,1,IPC_CREAT|0666);
	arg.val = 1;
	semctl(sem_x, 0, SETVAL, arg);

	sem_wait.sem_num = 0;
	sem_wait.sem_op = -1;
	sem_wait.sem_flg = SEM_UNDO;

	sem_signal.sem_num = 0;
	sem_signal.sem_op = 1;
	sem_signal.sem_flg = SEM_UNDO;

_INIT_OUT:
	return ret;
}

int main(int argc,char *argv[])
{
	int ret = 0;
	
	char c = 0;

	struct option longopts[]=
	{
        {"verbose",0,0,'v'},
		{"filename",1,0,'f'},
		{0,0,0,0}
	};

	while((c=getopt_long(argc,argv,"f:v",longopts,NULL))!=EOF){
		switch(c)
		{
		case 'f':
			mm_filename = argv[ optind -1 ];
			printf("mmap filename : [%d] %s\n",optind,mm_filename);
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			ret = 1;
			printf("Error : Wrong Option \" %c \"\n",c);
			goto _MAIN_OUT;
		}
	}

	if( init() ){
		ret = 1;
		printf("Error : init in main()\n");
		goto _MAIN_OUT;
	}

	run();

_MAIN_OUT:
	return 0;
}
