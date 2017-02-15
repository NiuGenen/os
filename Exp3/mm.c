#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<utime.h>
#include<getopt.h>

int verbose = 0;

int OPT = 0;
int FIFO = 0;
int LRU = 0;

char c = 0;

int seqSize = 0;
int mSize = 0;
int pageNum = 0;

int *seq;
int *m;
int cur_m_size = 0;

void showM()
{
	if(cur_m_size==0) return;
	printf("M");
	for(int i=0; i < cur_m_size; ++i)
	{
		printf("[%3d]",m[i]);
	}
}

int set_in(int *set,int i,int j,int k)
{
	if(i>j) return -1;
	while(i<=j)
	{
		if(set[i] == k) return i;
		++i;
	}
	return -1;
}
/*
int set_index(int *set,int size,int k)
{
	for(int i=0;i<size;++i)
	{
		if(set[i]==k) return i;
	}
	return -1;
}
*/
void fifo()
{
	int mp = 0;
	int con_lack_nr = 0;
	int lack_nr = 0;
	int target_nr = 0;

	cur_m_size = 0;

	if(verbose) printf("FIFO ALGORITHM\n");

	for(int i=0; i<seqSize; ++i)
	{
		if(verbose) printf("%2d	[%2d]",i,seq[i]);

		if(cur_m_size < mSize)
		{
			if( set_in(m, 0, cur_m_size-1, seq[i]) == -1 )
			{
				m[cur_m_size++] = seq[i];
				++lack_nr;
				++con_lack_nr;
				if(verbose) printf(" LACK ");
			}
			else
			{
				++target_nr;
				if(verbose) printf(" GETD ");
			}
		}
		else
		{
			if( set_in(m, 0, mSize-1, seq[i]) != -1)
			{
				++target_nr;
				if(verbose) printf(" GETD ");
			}
			else
			{
				m[mp] = seq[i];
				mp = (mp+1)%mSize;
				++lack_nr;
				if(verbose) printf(" LACK ");
			}
		}
		if(verbose)
		{
			showM();
			printf("\n");
		}
	}
	printf("CON_LACK_NR:    %5d\n",con_lack_nr);
	printf("TOTAL_LACK_NR:  %5d\n",lack_nr);
	printf("TARGET_NR:      %5d\n",target_nr);
}

int opt_choice(int *seq,int len,int pos,int *m,int mSize)
{
	int ret = -1;
	int mp = -1;
	int fount_mark = 0;
	int no_found_count = 0;

	for(int i=0;i<mSize;++i)
	{
		int j=0;
		for(j=pos+1;j<len;++j)
		{
			if(seq[j]==m[i])
			{
				fount_mark = 1;
				if(j>mp)
				{
					mp=j;
					ret=i;
					break;
				}
			}
		}
		if(!fount_mark)
		{
			mp = len;
			ret = i;
			++no_found_count;
		}
		if(no_found_count==2)
		{
			ret = -1;
			break;
		}
	}
	
	return ret;
}

void opt()
{
	int con_lack_nr = 0;
	int lack_nr = 0;
	int target_nr = 0;

	cur_m_size = 0;

	if(verbose) printf("OPT  ALGORITHM\n");

	for(int i=0; i<seqSize; ++i)
	{
		if(verbose) printf("%2d	[%2d]",i,seq[i]);

		if(cur_m_size < mSize)
		{
			if( set_in(m, 0, cur_m_size-1, seq[i]) == -1 )
			{
				m[cur_m_size++] = seq[i];
				++lack_nr;
				++con_lack_nr;
				if(verbose) printf(" LACK ");
			}
			else
			{
				++target_nr;
				if(verbose) printf(" GETD ");
			}
		}
		else
		{
			if( set_in(m, 0, mSize-1, seq[i]) != -1)
			{
				++target_nr;
				if(verbose) printf(" GETD ");
			}
			else
			{
				int pos = opt_choice(seq,seqSize,i,m,mSize);
				if(pos!=-1)
				{
					m[pos] = seq[i];
					++lack_nr;
					if(verbose) printf(" LACK ");
				}
				else
				{
					printf(" Cannot pridect an more\n");
					seqSize = i;
					break;
				}
			}
		}
		if(verbose)
		{
			showM();
			printf("\n");
		}
	}
	printf("CON_LACK_NR:    %5d\n",con_lack_nr);
	printf("TOTAL_LACK_NR:  %5d\n",lack_nr);
	printf("TARGET_NR:      %5d\n",target_nr);
}

int lru_choice(int *seq,int pos,int *m,int mSize)
{
	int ret = -1;

	int mp = 100000000;
	for(int i=0;i<mSize;++i)
	{
		for(int j=pos-1;j>=0;--j)
		{
			if(seq[j]==m[i])
			{
				if(j<mp)
				{
					mp = j;
					ret = i;
					break;
				}
			}
		}
	}
	return ret;
}

void lru()
{
	int con_lack_nr = 0;
	int lack_nr = 0;
	int target_nr = 0;

	cur_m_size = 0;

	if(verbose) printf("LRU  ALGORITHM\n");

	for(int i=0; i<seqSize; ++i)
	{
		if(verbose) printf("%2d	[%2d]",i,seq[i]);

		if(cur_m_size < mSize)
		{
			if( set_in(m, 0, cur_m_size-1, seq[i]) == -1 )
			{
				m[cur_m_size++] = seq[i];
				++lack_nr;
				++con_lack_nr;
				if(verbose) printf(" LACK ");
			}
			else
			{
				++target_nr;
				if(verbose) printf(" GETD ");
			}
		}
		else
		{
			if( set_in(m, 0, mSize-1, seq[i]) != -1 )
			{
				++target_nr;
				if(verbose) printf(" GETD ");
			}
			else
			{
				int pos = lru_choice(seq,i,m,mSize);
				if(pos!=-1)
				{
					m[pos] = seq[i];
					++lack_nr;
					if(verbose) printf(" LACK ");
				}
				else
				{
					printf("LRU CHOICE FAILED");
				}
			}
		}
		if(verbose)
		{
			showM();
			printf("\n");
		}
	}
	printf("CON_LACK_NR:    %5d\n",con_lack_nr);
	printf("TOTAL_LACK_NR:  %5d\n",lack_nr);
	printf("TARGET_NR:      %5d\n",target_nr);
}

void run()
{
	srand((unsigned)time(NULL));

	seq = (int *)malloc(sizeof(int)*seqSize);
	if(seq==NULL)
	{
		printf("Failed to create sequence!\n");
		goto _RUN_EXIT;
	}
	m = (int *)malloc(sizeof(int)*mSize);
	if(m==NULL)
	{
		printf("Failed to create memory!\n");
		goto _RUN_EXIT;
	}

	if(verbose) printf("Sequence: ");
	for(int i=0;i<seqSize;++i)
	{
		seq[i]=rand()%pageNum;
		if(verbose) printf(" %2d ",seq[i]);
	}
	if(verbose) printf("\n");

	if(OPT) opt();
	if(FIFO) fifo();
	if(LRU) lru();
	
_RUN_EXIT:
	if(seq) free(seq);
	if(m) free(m);
}

int main(int argc,char *argv[])
{
	struct option longopts[] =
	{
		{"opt",0,0,'o'},
		{"fifo",0,0,'f'},
		{"lru",0,0,'l'},
		{"verbose",0,0,'v'},
		{0,0,0,0}
	};

	while((c=getopt_long(argc,argv,"oflv",longopts,NULL))!=EOF)
	{
		switch(c)
		{
		case 'o':
			OPT = 1;
			break;
		case 'f':
			FIFO = 1;
			break;
		case 'l':
			LRU = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			break;
		}
	}
	
	printf("Enter SeqSize:");
	scanf("%d",&seqSize);
	printf("Enter MemorySize:");
	scanf("%d",&mSize);
	printf("Enter PageNum:");
	scanf("%d",&pageNum);
	
	printf("OPT: %2d FIFO: %2d LRU %2d\n",OPT,FIFO,LRU);
	printf("SeqSize: %3d MemorySize: %3d PageNum: %3d\n",seqSize,mSize,pageNum);

	run();
}
