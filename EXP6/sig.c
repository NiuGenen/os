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

int sem_s,sem_n,sem_e;
key_t key_s = 666,key_n=667,key_e=668;

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

int parbegin()
{
	int ret = 0;

	int sem_s_val = semctl(sem_s,0,GETVAL);
	int sem_n_val = semctl(sem_n,0,GETVAL);
	int sem_e_val = semctl(sem_e,0,GETVAL);
	printf("sem_s = %d\n",sem_s_val);
	printf("sem_n = %d\n",sem_n_val);
	printf("sem_e = %d\n",sem_e_val);

	int pid = fork();
	if(pid == 0){
		//father
		//producer
		int index = 0;
		char produce_char = 0;
		while(index<mm_buffer_size){
			produce_char = 'a'+index;;

			semop(sem_e,&sem_wait,1);	//buffer has empty
			semop(sem_s,&sem_wait,1);	//enter
			mm_buffer[index] = produce_char;
			printf("Produce: %c\n",produce_char);
			semop(sem_s,&sem_signal,1);	//retne
			semop(sem_n,&sem_signal,1);
			
			index += 1;
		}

		semctl(sem_s,0,IPC_RMID);
		semctl(sem_n,0,IPC_RMID);
		semctl(sem_e,0,IPC_RMID);
	}
	else if(pid > 0){
		//child
		//consumer
		int index = 0;
		char consume_char = 0;
		while(index<mm_buffer_size){
			
			semop(sem_n,&sem_wait,1);	//buffer has empty
			semop(sem_s,&sem_wait,1);	//enter
			consume_char = mm_buffer[index];
			index += 1;
			printf("Comsume: %c\n",consume_char);
			semop(sem_s,&sem_signal,1);	//retne
			semop(sem_e,&sem_signal,1);

		}
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

	union semun arg;

	sem_s = semget(key_s,1,IPC_CREAT|0666);
	arg.val = 1;
	semctl(sem_s, 0, SETVAL, arg);

	sem_n = semget(key_n,1,IPC_CREAT|0666);
	arg.val = 0;
	semctl(sem_n, 0, SETVAL, arg);

	sem_e = semget(key_e,1,IPC_CREAT|0666);
	arg.val = mm_buffer_size;
	semctl(sem_e, 0, SETVAL, arg);

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
