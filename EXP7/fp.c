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

int Resource[3],Available[3];
int Claim[4][3],Allocation[4][3];

int seq[9][4];

void printResource()
{
    printf("Resource : %2d %2d %2d\n",Resource[0],Resource[1],Resource[2]);
}

void printAvailable()
{
    printf("Availavle : %2d %2d %2d\n",Available[0],Available[1],Available[2]);
}

void printClaim()
{
    printf("Cliam\n");
    for(int i=0;i<4;++i){
        for(int j=0;j<3;++j){
            printf("%2d ",Claim[i][j]);
        }
        printf("\n");
    }
}

void printAllocation()
{
    printf("Allocation\n");
    for(int i=0;i<4;++i){
        for(int j=0;j<3;++j){
            printf("%2d ",Allocation[i][j]);
        }
        printf("\n");
    }
}

int finish[4];
int finish_size = 0;

int give(int pid,int *r)
{
    printResource();
    printAvailable();
    printClaim();
    printAllocation();

    printf("[%2d] request [%2d %2d %2d]\n",pid,r[0],r[1],r[2]);
    
    if( Allocation[pid][0] + r[0] > Claim[pid][0] ||
        Allocation[pid][1] + r[1] > Claim[pid][1] ||
        Allocation[pid][2] + r[2] > Claim[pid][2] )
    {
        printf("No [%d][%d %d %d]: request more than claim\n",pid,r[0],r[1],r[2]);
        return 1;
    }

    if( r[0]>Available[0] || r[1]>Available[1] || r[2]>Available[2] )
    {
        printf("No [%d][%d %d %d]: request more than available\n",pid,r[0],r[1],r[2]);
        return 1;
    }

    Available[0] -= r[0];
    Available[1] -= r[1];
    Available[2] -= r[2];

    Allocation[pid][0] += r[0];
    Allocation[pid][1] += r[1];
    Allocation[pid][2] += r[2];

    printAvailable();
    printClaim();
    printAllocation();

    int possible = 1;
    int safe_seq[4];
    int safe_i = 0;
    int set[4] = {0,1,2,3};
    int set_len = 4;
    int set_size = 4;
    if( finish_size > 0 ){
        for(int i=0;i<finish_size;++i){
            int fin = finish[i];
            for(int j=0;i<set_size;++j){
                if(set[j]==fin){
                    while(j<set_size-1) {set[j]=set[j+1];++j;}
                    set_size -= 1;
                    set_len -= 1;
                    break;
                }
            }
        }
    }
    printf("Set : ");
    for(int i=0;i<set_size;++i) printf("%2d ",set[i]);
    printf("\n");
    int ava[3] = {Available[0],Available[1],Available[2]};
    while(possible){
        int find=0;
        for(int i=0;i<set_size;++i){
            if( Claim[i][0] - Allocation[ set[i] ][0] <= ava[0] && 
                Claim[i][1] - Allocation[ set[i] ][1] <= ava[1] && 
                Claim[i][2] - Allocation[ set[i] ][2] <= ava[2] )
            {
                printf("Safe for %2d ",set[i]);
                safe_seq[safe_i] = set[i];
                safe_i += 1;

                ava[0] += Allocation[ set[i] ][0];
                ava[1] += Allocation[ set[i] ][1];
                ava[2] += Allocation[ set[i] ][2];

                for(int j=i;j<set_size-1;++j) set[j] = set[j+1];
                set_size -= 1;
                find=1;


                printf("ava[%2d %2d %2d]\n",ava[0],ava[1],ava[2]);
                break;
            }
        }
        if(!find){
            possible = 0;
        }
    }

    if(safe_i == set_len){
        printf("Safe_seq : ");
        for(int i=0; i<safe_i;++i) printf("%d ",safe_seq[i]);
        printf("\n");
    }
    else{
        printf("Not Safe!\n");
        Available[0] += r[0];
        Available[1] += r[1];
        Available[2] += r[2];

        Allocation[pid][0] -= r[0];
        Allocation[pid][1] -= r[1];
        Allocation[pid][2] -= r[2];
    }

    printResource();
    printAvailable();
    printClaim();
    printAllocation();

    if( Claim[pid][0] == Allocation[pid][0] && 
        Claim[pid][1] == Allocation[pid][1] && 
        Claim[pid][2] == Allocation[pid][2] )
    {
        printf("[%2d] Finished!\n",pid);

        finish[ finish_size++ ] = pid;

        Available[0] += Allocation[pid][0];
        Available[1] += Allocation[pid][1];
        Available[2] += Allocation[pid][2];

        Allocation[pid][0] = 0;
        Allocation[pid][1] = 0;
        Allocation[pid][2] = 0;

        printResource();
        printAvailable();
        printClaim();
        printAllocation();
    }
}

void run()
{
    for(int i=0;i<9;++i){
        int pid = seq[i][0];
        int r[3] = {seq[i][1],seq[i][2],seq[i][3]};
        give(pid,r);
        char c;
        printf("---------------------------------\n");
        scanf("%c",&c);
    }
}

int init()
{
    int ret = 0;

    Resource[0]=9;
    Resource[1]=3;
    Resource[2]=6;

    Available[0]=9;
    Available[1]=3;
    Available[2]=6;

    Claim[0][0]=3; Claim[0][1]=2; Claim[0][2]=2;
    Claim[1][0]=6; Claim[1][1]=1; Claim[1][2]=3;
    Claim[2][0]=3; Claim[2][1]=1; Claim[2][2]=4;
    Claim[3][0]=4; Claim[3][1]=2; Claim[3][2]=2;
    
    seq[0][0] = 0; seq[0][1]=1; seq[0][2]=0; seq[0][3]=0;
    seq[1][0] = 1; seq[1][1]=6; seq[1][2]=1; seq[1][3]=2;
    seq[2][0] = 2; seq[2][1]=2; seq[2][2]=1; seq[2][3]=1;
    seq[3][0] = 3; seq[3][1]=0; seq[3][2]=0; seq[3][3]=2;

    seq[4][0] = 0; seq[4][1]=2; seq[4][2]=2; seq[4][3]=2;
    seq[5][0] = 1; seq[5][1]=0; seq[5][2]=0; seq[5][3]=1;

    seq[6][0] = 0; seq[6][1]=2; seq[6][2]=2; seq[6][3]=2;

    seq[7][0] = 2; seq[7][1]=1; seq[7][2]=0; seq[7][3]=3;

    seq[8][0] = 3; seq[8][1]=4; seq[8][2]=2; seq[8][3]=0;

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
		{0,0,0,0}
	};

	while((c=getopt_long(argc,argv,"v",longopts,NULL))!=EOF){
		switch(c)
		{
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
