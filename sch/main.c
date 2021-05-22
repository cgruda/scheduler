/**
 * Computer Networks Exercise 2.
 * WRR, WDRR schedulers
 * written by Chaim Gruda & Nir Bieber.
 * June 2020
 */

#define _CRT_SECURE_NO_WARNINGS

#include "scheduler.h"
#include "tasks.h"
#include "queue.h"



/*===========================================================================*/
/*								  global vars def  							 */
/*===========================================================================*/

long int g_time = 0;
long int g_tx = 0;
long int g_stat_time = 0;



/*===========================================================================*/
/*								  		main	   							 */
/*===========================================================================*/

int main(int argc, char **argv)
{
	struct args args;
	struct files files;
	struct flow *flows[MAX_FLOWS] = { 0 };
	int flow_idx = -1;

	check_input(argc, argv, &args);
	open_files(&files, args.name);

	while (1)
	{
		// recive packet from file and add to flow
		rx(files.in, flows, args.size);

		// schedule a flow to transmit
		flow_idx = (*sch[args.sch])(flows);

		// buffer stats
		stat_buff(flows);

		// transmit packet
		tx(files.out, flows, flow_idx);

		// advance simulated time
		++g_time;

		// exit loop
		if (feof(files.in) && (g_tx == 0) && (nxt_prim_flow_idx(flows, 0) < 0))
			break;
	}

	// dump statistics
	dump_stats(files.stat, flows);

	// free resources
	close_files(&files);
	free_mem(flows);

	return 0;
}
