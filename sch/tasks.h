#ifndef _TASKS_H_
#define _TASKS_H_

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "scheduler.h"



/*===========================================================================*/
/*								  macros		   							 */
/*===========================================================================*/

/**
  ***************************************************************************
  * brief:  check error
  * input:  function name, resulr, error value
  ***************************************************************************
  */
#define CHECK_ERROR(func, res, err)												\
{																				\
	if (res == err)																\
	{																			\
		if (errno)																\
			printf("%s failed: %s\n", func, strerror(errno));					\
		else																	\
			printf("%s failed: unknown error\n", func);							\
		exit(EXIT_FAILURE);														\
	}																			\
}



/*===========================================================================*/
/*								  structs		   							 */
/*===========================================================================*/

struct args
{
	int sch;
	char *name;
	int size;
};

struct files
{
	FILE *in;
	FILE *out;
	FILE *stat;
};



/*===========================================================================*/
/*								  function dec	   							 */
/*===========================================================================*/

void check_input(int argc, char **argv, struct args *args);
void open_files(struct files *files, char *name);
void close_files(struct files *files);
void dump_stats(FILE *fp, struct flow *flows[]);
void free_mem(struct flow *flows[]);



#endif // _TASKS_H_