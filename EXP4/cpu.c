#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<utime.h>
#include<getopt.h>

int verbose = 0;

int arive_t;
int server_t;
int process_nr;

int t=0;

typedef struct LIST
{
	int *process;
	int length;
}LIST;
LIST list_wait,list_over;

int empty_wait(){
	return list_wait.length==0;
}

int process_wait(int p)
{
	list_wait.process[ list_wait.length++ ] = p;
}

int process_run(int p)
{
	for(int i=0;i<list_wait.length;++i){
		if(list_wait.process[i] == p){
			for(int j=i;j<list_wait.length-1;++j){
				list_wait.process[j]=list_wait.process[j+1];
			}
			list_wait.length -= 1;
			break;
		}
	}
}

int process_over(int p)
{
	list_over.process[ list_over.length++ ] = p;
}

typedef struct _Process
{
	int arive;//arive
	int server;//s
	int left_t;
	int over_t;//finish
	int wait_t;//w=finish-arive-s
	float rate;//=(w+s)/s
}_Process;
_Process * seq;

int init()
{
	srand((unsigned)time(NULL));
	
	seq = (_Process*)malloc(sizeof(_Process)*process_nr);
	if(seq==NULL)
	{
		perror("Cannot malloc process!\n");
		return 1;
	}
	
	list_wait.process = (int*)malloc(sizeof(int)*process_nr);
	list_over.process = (int*)malloc(sizeof(int)*process_nr);
	list_wait.length = 0;
	list_over.length = 0;
	
	return 0;
}

int side_process()
{//adjust process's arive time
	int minaP = 0;
	for(int i=1;i<process_nr;++i)
	{
		if(seq[i].over_t == -1 && seq[i].arive < seq[minaP].arive)
			minaP = i;
	}
	int minus_time = seq[minaP].arive;
	for(int i=0;i<process_nr;++i)
	{
		seq[i].arive -= minus_time;
	}
	return 0;
}

int random_seq()
{
	if(verbose)
		printf("Process  Arive  Server\n");
		
	int i=0;
	for(;i<process_nr;++i){
		seq[i].arive = rand()%arive_t + 1;
		seq[i].server = rand()%server_t + 1;
		seq[i].over_t = -1;
		seq[i].left_t = seq[i].server;
	}
	
	side_process();
	for(int i=0;i<process_nr;++i){
		printf("      %c  %5d  %6d\n",'A'+i,seq[i].arive,seq[i].server);
	}
	
	return 0;
}

int printResult()
{
	printf("Process Arive Server Over Wait Rate\n");
	for(int i=0;i<process_nr;++i){
		int k = list_over.process[i];
		printf("      %c %5d %6d %4d %4d %f\n",'A'+k,seq[k].arive,seq[k].server,seq[k].over_t,seq[k].wait_t,seq[k].rate);
	}
	return 0;
}

int reset_seq()
{
	t = 0;
	for(int i=0;i<process_nr;++i){
		seq[i].left_t = seq[i].server;
		seq[i].over_t = -1;
		seq[i].wait_t = 0;
		seq[i].rate = 0;
	}
}

int FCFS_next()
{
	if( list_wait.length == 0 ) return -1;
		
	int next = list_wait.process[0];
	for(int i=0;i<list_wait.length-1;++i){
		list_wait.process[i] = list_wait.process[i+1];
	}
	list_wait.length -= 1;
	return next;
}

int FCFS()//first come first server
{
	printf("\n-----FCFS-----\n");
	
	t=0;
	list_wait.length = 0;
	list_over.length = 0;
	reset_seq();
	
	if(verbose) printf("StartTime Process Arive Server Left EndTime NewProcess Action\n");
	
	int curP = -1;
	int count = 0;
	int preP = -1,dispatch = 1;
	while( count < process_nr ){//clock t start at 0
	
		int flag = 0;
		for(int i=0; i<process_nr; ++i){//process coming and waiting
			if(seq[i].over_t == -1 && seq[i].arive == t){
				process_wait(i);
				++flag;
				if(t>0 &&flag<=3) printf("%c ",'A'+i);
			}
		}
		if(t>0 &&flag>=3) printf(" ... ");
		else if(t>0 &&flag==2) printf("       ");
		else if(t>0 &&flag==1) printf("         ");
		else if(t>0 &&flag==0) printf("           ");
		
		if(dispatch) curP = FCFS_next();//cpu choose next process, if no process then return -1
		
		if(verbose && t>0){//dispatching info
			if( preP != -1 && dispatch && curP != -1 ) printf("%c -> %c",'A'+preP,'A'+curP);
			if( preP != -1 && dispatch && curP == -1 ) printf("%c -> cpu waiting",'A'+preP);
			if( preP == -1 && dispatch && curP != -1 ) printf("%c <- cpu waiting",'A'+curP);
			printf("\n");
		}
		
		if(curP != -1 && t >= seq[curP].arive){//process running
			seq[curP].left_t -= 1;
			if(seq[curP].left_t < 0){
				printf("[ERROR] invalid process record in left_t\n");
				count = process_nr;
				continue;
			}
		}
		
		++t;//tick tock
		
		if(curP != -1 && seq[curP].left_t == 0){//process finish
			preP = curP;
			dispatch = 1;
			++count;
			
			seq[curP].over_t = t;
			seq[curP].wait_t = seq[curP].over_t - seq[curP].arive - seq[curP].server;
			seq[curP].rate = seq[curP].wait_t / (float)(seq[curP].server) + 1;

			process_over(curP);
			
		}else if(curP != -1 && seq[curP].left_t != 0){//process not finish
			dispatch = 0;
			
		}else if(curP == -1){//cpu waiting
			dispatch = 1;
			preP = -1;
		}
		
		if(verbose){//processing info
			if( curP != -1)		printf("%9d       %c %5d %6d %4d %7d ", t-1, 'A'+curP, seq[curP].arive, seq[curP].server, seq[curP].left_t,t);
			else				printf("%9d    C P U W A I T I N G    %7d ",t-1,t);
		}
	}
	printf("\n");
	
	printResult();
	return 0;
}


int RR1_next()
{
	if(list_wait.length == 0) return -1;
	
	int next = list_wait.process[0];
	for(int i=0;i<list_wait.length-1;++i) {
		list_wait.process[i] = list_wait.process[i+1];
	}
	list_wait.length -= 1;
	return next;
}

#define RR1_TIME 1

int RR1()//q=1
{
	printf("\n----- RR1 -----\n");
	
	t=0;
	list_wait.length = 0;
	list_over.length = 0;
	reset_seq();
	
	if(verbose) printf("StartTime Process Arive Server Left EndTime NewProcess Action\n");
	
	int curP = -1;
	int count = 0;
	int preP = -1,dispatch = 1;
	int q = RR1_TIME;
	while(count < process_nr){//clock t start at 0
	
		int flag = 0;
		for(int i=0; i<process_nr; ++i){//process coming and waiting
			if(seq[i].over_t == -1 && seq[i].arive == t){
				process_wait(i);
				++flag;
				if(t>0 &&flag<=3) printf("%c ",'A'+i);
			}
		}
		if(t>0 &&flag>=3) printf(" ... ");
		else if(t>0 &&flag==2) printf("       ");
		else if(t>0 &&flag==1) printf("         ");
		else if(t>0 &&flag==0) printf("           ");
		
		if( q==0 ){
			q = RR1_TIME;
			if(seq[curP].left_t != 0) process_wait(curP);
		}
		
		if(dispatch) curP = RR1_next();//cpu choose next process, if no process then return -1
		
		if(verbose && t>0){//dispatching info
			if( preP != -1 && dispatch && curP != -1 ) printf("%c -> %c",'A'+preP,'A'+curP);
			if( preP != -1 && dispatch && curP == -1 ) printf("%c -> cpu waiting",'A'+preP);
			if( preP == -1 && dispatch && curP != -1 ) printf("%c <- cpu waiting",'A'+curP);
			printf("\n");
		}
		
		if(curP != -1 && t >= seq[curP].arive){//process running
			seq[curP].left_t -= 1;
			if(seq[curP].left_t < 0){
				printf("[ERROR] invalid process record in left_t\n");
				count = process_nr;
				continue;
			}
			--q;
		}
		
		++t;//tick tock
		
		if(curP != -1 && ( q == 0 || seq[curP].left_t == 0 ) ){//process finish or time clip used out
			preP = curP;
			dispatch = 1;
			
			if(seq[curP].left_t == 0){
				++count;
				seq[curP].over_t = t;
				seq[curP].wait_t = seq[curP].over_t - seq[curP].arive - seq[curP].server;
				seq[curP].rate = seq[curP].wait_t / (float)(seq[curP].server) + 1;

				process_over(curP);
			}
			
		}else if(curP != -1 && seq[curP].left_t != 0){//process not finish
			dispatch = 0;
			
		}else if(curP == -1){//cpu waiting
			dispatch = 1;
			preP = -1;
		}
		
		if(verbose){//processing info
			if( curP != -1)		printf("%9d       %c %5d %6d %4d %7d ", t-1, 'A'+curP, seq[curP].arive, seq[curP].server, seq[curP].left_t,t);
			else				printf("%9d    C P U W A I T I N G    %7d ",t-1,t);
		}
	}
	printf("\n");
	
	printResult();
	return 0;
}

int SPN_next()//SPN strategy
{
	if(list_wait.length == 0) return -1;
	
	int next = list_wait.process[0],index = 0;
	for(int i=1; i<list_wait.length; ++i){
		int this = list_wait.process[i];
		if( seq[ this ].server < seq[ next ].server ){//found a shorter process
			next = this;
			index = i;
		}
	}
	for(int i=index; i<list_wait.length-1; ++i){
		list_wait.process[i] = list_wait.process[i+1];
	}
	list_wait.length -= 1;
	return next;
}

int SPN()//shortest process first
{
	printf("\n-----SPN-----\n");
	
	t=0;
	list_wait.length = 0;
	list_over.length = 0;
	reset_seq();
	
	if(verbose) printf("StartTime Process Arive Server Left EndTime NewProcess Action\n");
	
	int curP = -1;
	int count = 0;
	int preP = -1,dispatch = 1;
	while( count < process_nr ){//clock t start at 0
	
		int flag = 0;
		for(int i=0; i<process_nr; ++i){//process coming and waiting
			if(seq[i].over_t == -1 && seq[i].arive == t){
				process_wait(i);
				++flag;
				if(t>0 &&flag<=3) printf("%c ",'A'+i);
			}
		}
		if(t>0 &&flag>=3) printf(" ... ");
		else if(t>0 &&flag==2) printf("       ");
		else if(t>0 &&flag==1) printf("         ");
		else if(t>0 &&flag==0) printf("           ");
		
		if(dispatch) curP = SPN_next();//cpu choose next process, if no process then return -1
		
		if(verbose && t>0){//dispatching info
			if( preP != -1 && dispatch && curP != -1 ) printf("%c -> %c",'A'+preP,'A'+curP);
			if( preP != -1 && dispatch && curP == -1 ) printf("%c -> cpu waiting",'A'+preP);
			if( preP == -1 && dispatch && curP != -1 ) printf("%c <- cpu waiting",'A'+curP);
			printf("\n");
		}
		
		if(curP != -1 && t >= seq[curP].arive){//process running
			seq[curP].left_t -= 1;
			if(seq[curP].left_t < 0){
				printf("[ERROR] invalid process record in left_t\n");
				count = process_nr;
				continue;
			}
		}
		
		++t;//tick tock
		
		if(curP != -1 && seq[curP].left_t == 0){//process finish
			preP = curP;
			dispatch = 1;
			++count;
			
			seq[curP].over_t = t;
			seq[curP].wait_t = seq[curP].over_t - seq[curP].arive - seq[curP].server;
			seq[curP].rate = seq[curP].wait_t / (float)(seq[curP].server) + 1;

			process_over(curP);
			
		}else if(curP != -1 && seq[curP].left_t != 0){//process not finish
			dispatch = 0;//not allow seize
			
		}else if(curP == -1){//cpu waiting
			dispatch = 1;
			preP = -1;
		}
		
		if(verbose){//processing info
			if( curP != -1)		printf("%9d       %c %5d %6d %4d %7d ", t-1, 'A'+curP, seq[curP].arive, seq[curP].server, seq[curP].left_t,t);
			else				printf("%9d    C P U W A I T I N G    %7d ",t-1,t);
		}
	}
	printf("\n");
	
	printResult();
	return 0;
}

int SRT_next()
{
	if(list_wait.length == 0) return -1;

	int next = list_wait.process[0], index=0;
	for(int i=1; i<list_wait.length; ++i){
		int this = list_wait.process[i];
		if( seq[ this ].left_t < seq[ next ].left_t ){
			next = this;
			index = i;
		}
	}
	for(int i=index; i<list_wait.length-1; ++i){
		list_wait.process[i] = list_wait.process[i+1];
	}
	list_wait.length -= 1;
	return next;
}

int SRT()
{
	printf("\n-----SRT-----\n");
	
	t=0;
	list_wait.length = 0;
	list_over.length = 0;
	reset_seq();
	
	if(verbose) printf("StartTime Process Arive Server Left EndTime NewProcess Action\n");
	
	int curP = -1;
	int count = 0;
	int preP = -1,dispatch = 1;
	while( count < process_nr ){//clock t start at 0
	
		int flag = 0;
		for(int i=0; i<process_nr; ++i){//process coming and waiting
			if(seq[i].over_t == -1 && seq[i].arive == t){
				process_wait(i);
				++flag;
				if(t>0 &&flag<=3) printf("%c ",'A'+i);
			}
		}
		if(t>0 &&flag>=3) printf(" ... ");
		else if(t>0 &&flag==2) printf("       ");
		else if(t>0 &&flag==1) printf("         ");
		else if(t>0 &&flag==0) printf("           ");
		
		if(dispatch){//cpu choose next process, if no process then return -1
			if(curP!=-1 && seq[curP].left_t > 0){//process seize
				int temp = SRT_next();
				if(temp != -1 && seq[curP].left_t > seq[temp].left_t){
					process_wait(curP);
					curP = temp;
				}
				else if(temp != -1){
					process_wait(temp);
					dispatch = 0;
				}
			}
			else curP = SRT_next();
		}
		
		if(verbose && t>0){//dispatching info
			if( preP != -1 && dispatch && curP != -1 ) printf("%c -> %c",'A'+preP,'A'+curP);
			if( preP != -1 && dispatch && curP == -1 ) printf("%c -> cpu waiting",'A'+preP);
			if( preP == -1 && dispatch && curP != -1 ) printf("%c <- cpu waiting",'A'+curP);
			printf("\n");
		}
		
		if(curP != -1 && t >= seq[curP].arive){//process running
			seq[curP].left_t -= 1;
			if(seq[curP].left_t < 0){
				printf("[ERROR] invalid process record in left_t\n");
				count = process_nr;
				continue;
			}
		}
		
		++t;//tick tock
		
		if(curP != -1 && seq[curP].left_t == 0){//process finish
			preP = curP;
			dispatch = 1;
			++count;
			
			seq[curP].over_t = t;
			seq[curP].wait_t = seq[curP].over_t - seq[curP].arive - seq[curP].server;
			seq[curP].rate = seq[curP].wait_t / (float)(seq[curP].server) + 1;

			process_over(curP);
			
		}else if(curP != -1 && seq[curP].left_t != 0){//process not finish
			preP = curP;
			dispatch = 1;//allow seize
			
		}else if(curP == -1){//cpu waiting
			dispatch = 1;
			preP = -1;
		}
		
		if(verbose){//processing info
			if( curP != -1)		printf("%9d       %c %5d %6d %4d %7d ", t-1, 'A'+curP, seq[curP].arive, seq[curP].server, seq[curP].left_t,t);
			else				printf("%9d    C P U W A I T I N G    %7d ",t-1,t);
		}
	}
	printf("\n");
	
	printResult();
	return 0;
}

int HRRN_next()
{
	if(list_wait.length == 0) return -1;

	int next = list_wait.process[0], index=0;
	float hrrn = (float)(seq[next].wait_t) / seq[ next ].server;
	for(int i=1; i<list_wait.length; ++i){
		int this = list_wait.process[i];
		if( ( (float)(seq[ this ].wait_t) / seq[ this ].server ) > hrrn ){
			next = this;
			index = i;
		}
	}
	for(int i=index; i<list_wait.length-1; ++i){
		list_wait.process[i] = list_wait.process[i+1];
	}
	list_wait.length -= 1;
	return next;
}

int HRRN()
{
	printf("\n-----HRRN-----\n");
	
	t=0;
	list_wait.length = 0;
	list_over.length = 0;
	reset_seq();
	
	if(verbose) printf("StartTime Process Arive Server Left EndTime NewProcess Action\n");
	
	int curP = -1;
	int count = 0;
	int preP = -1,dispatch = 1;
	while( count < process_nr ){//clock t start at 0
	
		int flag = 0;
		for(int i=0; i<process_nr; ++i){//process coming and waiting
			if(seq[i].over_t == -1 && seq[i].arive == t){
				process_wait(i);
				++flag;
				if(t>0 &&flag<=3) printf("%c ",'A'+i);
			}
		}
		if(t>0 &&flag>=3) printf(" ... ");
		else if(t>0 &&flag==2) printf("       ");
		else if(t>0 &&flag==1) printf("         ");
		else if(t>0 &&flag==0) printf("           ");
		
		if(dispatch){//cpu choose next process, if no process then return -1
			if(curP != -1 && seq[curP].left_t > 0){//process seize
				int temp = HRRN_next();
				if( temp != -1 && ((float)(seq[curP].wait_t)/seq[curP].server) < ((float)(seq[temp].wait_t)/seq[temp].server) ){
					process_wait(curP);
					curP = temp;
				}
				else if(temp != -1){
					dispatch = 0;
					process_wait(temp);
				}
			}
			else curP = HRRN_next();
		}	
		
		if(verbose && t>0){//dispatching info
			if( preP != -1 && dispatch && curP != -1 ) printf("%c -> %c",'A'+preP,'A'+curP);
			if( preP != -1 && dispatch && curP == -1 ) printf("%c -> cpu waiting",'A'+preP);
			if( preP == -1 && dispatch && curP != -1 ) printf("%c <- cpu waiting",'A'+curP);
			printf("\n");
		}
		
		if(curP != -1 && t >= seq[curP].arive){//process running
			seq[curP].left_t -= 1;
			if(seq[curP].left_t < 0){
				printf("[ERROR] invalid process record in left_t\n");
				count = process_nr;
				continue;
			}
		}
		
		++t;//tick tock
		
		for(int i=0; i<list_wait.length; ++i){
			seq[ list_wait.process[i] ].wait_t += 1;
		}
		
		if(curP != -1 && seq[curP].left_t == 0){//process finish
			preP = curP;
			dispatch = 1;
			++count;
			
			seq[curP].over_t = t;
			seq[curP].wait_t = seq[curP].over_t - seq[curP].arive - seq[curP].server;
			seq[curP].rate = seq[curP].wait_t / (float)(seq[curP].server) + 1;

			process_over(curP);
			
		}else if(curP != -1 && seq[curP].left_t != 0){//process not finish
			preP = curP;
			dispatch = 1;//allow seize
			
		}else if(curP == -1){//cpu waiting
			dispatch = 1;
			preP = -1;
		}
		
		if(verbose){//processing info
			if( curP != -1)		printf("%9d       %c %5d %6d %4d %7d ", t-1, 'A'+curP, seq[curP].arive, seq[curP].server, seq[curP].left_t,t);
			else				printf("%9d    C P U W A I T I N G    %7d ",t-1,t);
		}
	}
	printf("\n");
	
	printResult();
	return 0;
}

int run()
{
	FCFS();
	RR1();
	SPN();
	SRT();
	HRRN();
	return 0;
}

int main(int argc,char*argv[])
{
	char c = 0;

	struct option longopts[]=
	{
		{"arive",1,0,'a'},
		{"process",1,0,'p'},
		{"server",1,0,'s'},
		{"verbose",0,0,'v'},
		{0,0,0,0}
	};

	while((c=getopt_long(argc,argv,"a:p:s:v",longopts,NULL))!=EOF){
		switch(c)
		{
		case 'v':
			verbose = 1;
			break;
		case 'a':
			arive_t = atoi(optarg);
			break;
		case 'p':
			process_nr = atoi(optarg);
			break;
		case 's':
			server_t = atoi(optarg);
			break;
		default:
			printf("Wrong Option!\n");
			goto _MAIN_OVER;
			break;
		}
	}

	if(verbose){
		printf("arive_t_max:%d\n",arive_t);
		printf("server_t_max:%d\n",server_t);
		printf("process_nr_max:%d\n",process_nr);
	}

	if(init()) goto _MAIN_OVER;
	if(random_seq()) goto _MAIN_OVER;
	if(run()) goto _MAIN_OVER;;
	
_MAIN_OVER:
	return 0;
}
