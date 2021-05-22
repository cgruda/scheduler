#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_



#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "queue.h"


#define MAX_FLOWS			(32000 + 1)
#define MAX_ADDR_LEN		((3 * 4) + 3 + 1)
#define MAX_INPUT_LINE_LEN	200


/*===========================================================================*/
/*								  enumerals		   							 */
/*===========================================================================*/

// enumerate schedulers
enum schedulers
{
	SCH_WRR,
	SCH_WDRR,
	SCH_MAX
};



/*===========================================================================*/
/*							 global vars dec	  							 */
/*===========================================================================*/

// time vars
long int g_time;
long int g_tx;
long int g_stat_time;

// scheduler functions arr
int(*sch[SCH_MAX])(struct flow *flows[]);



/*===========================================================================*/
/*								  structures	   							 */
/*===========================================================================*/

struct packet
{
	struct flow *flow;
	long int packt_id;
	long int arrival_time;
	unsigned short length;
	TAILQ_ENTRY(packet) q;
};


struct flow_stats
{
	// stream statistics
	int num_packets;
	long int max_delay;
	int max_buff;
	double avg_delay;
	double avg_buff;

	// for calculations
	long long acmltd_buff;
	long long acmltd_delay;	
};


struct flow_info
{
	char src_add[MAX_ADDR_LEN];
	char dst_add[MAX_ADDR_LEN];
	unsigned short src_port;
	unsigned short dst_port;
};


struct flow
{
	// flow info
	struct flow_info info;
	int weight;
	int credit;

	// packet queues
	int pkts_in_q;
	TAILQ_HEAD(pkt_q, packet) pkt_q;

	// stats
	struct flow_stats stats;
};



/*===========================================================================*/
/*								  functions dec	   							 */
/*===========================================================================*/

void push_packet(struct flow *flow, struct packet *packet);
void pop_packet(struct flow *flow);
void init_flow(struct flow *flows[], int idx, struct flow_info *info, int weight);
void stat_buff(struct flow *flows[]);
void rx(FILE *fp, struct flow *flows[], int dflt_w);
void tx(FILE *fp, struct flow *flows[], int flow_idx);
bool flows_eq(struct flow_info *a, struct flow_info *b);
int scan_packet(FILE *fp, struct packet *pkt, struct flow_info *f_info, int *weight, int dflt_w);
int sch_WRR(struct flow *flows[]);
int sch_WDRR(struct flow *flows[]);
int nxt_prim_flow_idx(struct flow *flows[], int c_flow_idx);



#endif // _SCHEDULER_H_