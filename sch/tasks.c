
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "tasks.h"
#include "scheduler.h"


#define ARG_NUM			4
#define NAME_EXTENTION	11
#define ARG_WRR			"RR"
#define ARG_WDRR		"DRR"

/*===========================================================================*/
/*								  function def	   							 */
/*===========================================================================*/

/**
  ***************************************************************************
  * brief:  check input
  * input:  argc and argv from main
  ***************************************************************************
  */
void check_input(int argc, char **argv, struct args *args) 
{
	bool err = false;
	if (argc != ARG_NUM)
	{
		printf("Input Error: 3 arguments expected, %d received.\n", argc - 1);
		err = true;
	}
	if (!(!strcmp(argv[1], ARG_WRR) || !strcmp(argv[1], ARG_WDRR)))
	{
		printf("Input Error: \"%s\" is not a valid scheduler type.\n", argv[1]);
		err = true;
	}
	for (int i = 0; argv[3][i]; ++i)
	{
		if (!isdigit(argv[3][i]))
		{
			printf("Input Error: \"%s\" is not a valid size.\n", argv[3]);
			err = true;
		}
	}

	if (err)
		exit(EXIT_FAILURE);

	if (!strcmp(argv[1], ARG_WRR))
		args->sch = SCH_WRR;
	else // "DRR"
		args->sch = SCH_WDRR;

	args->name = argv[2];
	args->size = strtol(argv[3], NULL, 10);
}


/**
  ***************************************************************************
  * brief:  open files
  * input:  files pointer, base name for generating file pathes
  ***************************************************************************
  */
void open_files(struct files *files, char *name)
{
	int name_len = (int)strlen(name);
	bool res;

	// alocate mem
	char *in_path = malloc(name_len + NAME_EXTENTION);
	char *out_path = malloc(name_len + NAME_EXTENTION);
	char *stat_path = malloc(name_len + NAME_EXTENTION);
	res = (!in_path || !out_path || !stat_path);
	CHECK_ERROR("malloc()", res, true);
	
	// genrate full pathes
	sprintf(in_path, "%s_in.txt", name);
	sprintf(out_path, "%s_out.txt", name);
	sprintf(stat_path, "%s_stat.txt", name);

	// open files
	files->in   = fopen(in_path, "r");
	files->out  = fopen(out_path, "w");
	files->stat = fopen(stat_path, "w");
	res = (!files->in || !files->out || !files->stat);
	CHECK_ERROR("fopen()", res, true);
	
	// free mem
	free(in_path);
	free(out_path);
	free(stat_path);
}


/**
  ***************************************************************************
  * brief:  close files
  * input:  files struct
  ***************************************************************************
  */
void close_files(struct files *files)
{
	if (fclose(files->in)  ||
		fclose(files->out) ||
		fclose(files->stat))
	{
		// forced exit with error message
		CHECK_ERROR("fclose()", 0, 0);
	}
}


/**
  ***************************************************************************
  * brief:  dump flow statistics in stats file
  * input:  file pointer, flows array
  ***************************************************************************
  */
void dump_stats(FILE *fp, struct flow *flows[])
{
	g_stat_time = g_time - g_stat_time;

	struct flow_stats *stats;
	struct flow_info  *info;

	for (int i = 0; flows[i]; ++i)
	{
		stats = &flows[i]->stats;
		info  = &flows[i]->info;

		// sanity check - prevnet div by 0
		CHECK_ERROR("dump_stats()", (!stats->num_packets || !g_stat_time), 1);
		
		// calc stats
		stats->avg_delay = (double)stats->acmltd_delay / (double)stats->num_packets;
		stats->avg_buff = (double)stats->acmltd_buff / (double)g_stat_time;

		// dump to file
		fprintf(fp, "%s %hu %s %hu %li %li %.2lf %i %.2lf\n",
					info->src_add, info->src_port,
					info->dst_add, info->dst_port,
					stats->num_packets,
					stats->max_delay,
					stats->avg_delay,
					stats->max_buff,
					stats->avg_buff);
	}
}


/**
  ***************************************************************************
  * brief:  free allocated memory
  * input:  flows array
  ***************************************************************************
  */
void free_mem(struct flow *flows[])
{
	for (int i = 0; flows[i]; ++i)
		free(flows[i]);
}
