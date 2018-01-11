// caltable_standarize.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

typedef struct
{
	long           freq;
	double         power;
}node_s;

node_s g_samples[] =
{
	{ 2506, 24.5   },
	{ 2593, 24.0   },
	{ 2680, 23.3    },
};

double get_samples(long freq)
{
	int   cnt = sizeof(g_samples) / sizeof(node_s);
	int   node1 = 0;
	int   node2 = 0;

	for (node1 = 0; node1 < cnt - 1; node1++)
	{
		if (freq < g_samples[0].freq)
		{
			node1 = 0;
			break;
		}

		if (freq > g_samples[cnt - 1].freq)
		{
			node1 = cnt - 2;
			break;
		}

		if ( (freq >= g_samples[node1].freq) && (freq <= g_samples[node1 + 1].freq) )
		{
			break;
		}


	}

	if (node1 >= cnt)
	{
		node1 = cnt - 2;
	}

	node2 = node1 + 1;

	

	long    freq_delta = freq - g_samples[node1].freq;
	double  pwr_new = g_samples[node1].power + (double)freq_delta * (g_samples[node2].power - g_samples[node1].power) / (double)(g_samples[node2].freq - g_samples[node1].freq);

	printf("cacl by [%ld ~ %ld] freq[%ld] power[%f]\r\n", g_samples[node1].freq, g_samples[node2].freq, freq, pwr_new);
	return pwr_new;
}


int _tmain(int argc, _TCHAR* argv[])
{
	for (long freq = 2496; freq <= 2690; freq += 10)
	{
		get_samples(freq);
	}

	getchar();
	return 0;
}

