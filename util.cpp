
#include "util.h"

timer *timer_start(const char *name)
{
	timer *tm = (timer *)malloc(sizeof(timer));
	tm->name = (char *)malloc(strlen(name) * sizeof(char));
	strcpy(tm->name, name);
	tm->start = omp_get_wtime();
	// printf("Timer [%s] set.\n", tm->name);
	return tm;
}

void timer_end(timer *tm)
{
	tm->end = omp_get_wtime();
	printf("%.7f \t\t : %s \n", (tm->end - tm->start), tm->name);
	free(tm->name);
	free(tm);
}

void print_trace(void)
{
	void *callstack[128];
	int i, frames = backtrace(callstack, 128);
	char **strs = backtrace_symbols(callstack, frames);
	for (i = 0; i < frames; ++i)
	{
		printf("%s\n", strs[i]);
	}
	free(strs);
}

void print_vec(int n, int *vec, const char *name)
{
	int i;

	printf("______________________________\n");
	printf("%s (size = %d) : \n", name, n);
	for (i = 0; i < n; i++)
		printf("%d ", vec[i]);
	printf("\n-----------------------------\n");
}

void printusage()
{
	printf("usage: featen [options] \n");
	
	printf("\t-i input : input tensor path\n");
	printf("\t-o out : feature info output file\n");
	printf("\t-m method : feature extraction method\n\t\tOptions:{map, sort, fragment, hybrid}\n");
	printf("\t-c csv_out : feture out file to be in csv (1) or json (0) format \n");
	printf("\t-d only3d : features to be in only 3 dim (1) or N-dim (0) \n");
	

	exit(1);
}
