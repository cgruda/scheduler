
#define _CRT_SECURE_NO_WARNINGS

#include "scheduler.h"
#include "tasks.h"


/*===========================================================================*/
/*								  function def	   							 */
/*===========================================================================*/

/**
  ***************************************************************************
  * brief:  return idx of next senior non-empty flow
  * input:  flows array, starting idx to strat search from
  * output: flow idx, -1 if non found
  ***************************************************************************
  */
int nxt_prim_flow_idx(struct flow *flows[], int strt_idx)
{
	int nxt_idx = 0;

	// loop from requested idx
	for (nxt_idx = strt_idx; flows[nxt_idx]; ++nxt_idx)
		if(flows[nxt_idx]->pkts_in_q > 0)
			return nxt_idx;
	
	// loop from idx 0 up to strt idx
	for (nxt_idx = 0; flows[nxt_idx] && (nxt_idx <= strt_idx); ++nxt_idx)
		if(flows[nxt_idx]->pkts_in_q > 0)
			return nxt_idx;
	
	// none found
	return -1;
}


/**
  ***************************************************************************
  * brief:  compare flow infos for equality
  * input:  flow infos to be compared
  * output: true or false
  ***************************************************************************
  */
bool flows_eq(struct flow_info *a, struct flow_info *b)
{
	if (a->dst_port == b->dst_port &&
		a->src_port == b->src_port &&
		!strcmp(a->dst_add, b->dst_add) &&
		!strcmp(a->src_add, b->src_add))
		return true;
	else
		return false;
}


/**
  ***************************************************************************
  * brief:  "rx" a packet from input file to flow queue
  * input:  input file pointer, flows array, default weight
  ***************************************************************************
  */
void rx(FILE *fp, struct flow *flows[], int dflt_w)
{	
	// skip if file empty
	if (feof(fp))
		return;

	// rx packets
	do {
		int res = 0;
		int weight = 0;
		int flow_idx = 0;
		struct flow_info flow_info = { 0 };
		struct packet *pkt = calloc(1, sizeof(*pkt));
		CHECK_ERROR("calloc()", pkt, NULL);
		
		// save file position & scan line
		long position = ftell(fp);
		CHECK_ERROR("ftell()", position, EOF)
		res = scan_packet(fp, pkt, &flow_info, &weight, dflt_w);

		// take care of '\n' before eof
		if (res == 0)
		{
			free(pkt);
			return;
		}

		// packet is "from future" - rewind file
		if (pkt->arrival_time > g_time)
		{	
			int res = !!fseek(fp, position, SEEK_SET);
			CHECK_ERROR("fseek()", res, 1);
			free(pkt);
			return;
		}

		// find flow for packet
		for (; flow_idx < MAX_FLOWS && flows[flow_idx]; ++flow_idx)
			if (flows_eq(&flow_info, &flows[flow_idx]->info))
				break;
		CHECK_ERROR("rx()", flow_idx, MAX_FLOWS);


		// init new flow if needed
		if (!flows[flow_idx])
			init_flow(flows, flow_idx, &flow_info, weight);

		// insert packet to flow
		push_packet(flows[flow_idx], pkt);

	} while (!feof(fp));
}


/**
  ***************************************************************************
  * brief:  schedule next flow using WRR algorithem
  * input:  flows array
  * output: idx of scheduled flow, -1 if none found
  ***************************************************************************
  */
int sch_WRR(struct flow *flows[]) 
{
	static int flow_idx = -1;
	static int w = 0;

	// no flow has dibs
	if (flow_idx < 0)
	{
		flow_idx = nxt_prim_flow_idx(flows, 0);
		if (flow_idx < 0)
			return -1;

		w = flows[flow_idx]->weight;
	}

	// select next flow to tx
	if (g_tx <= 1)
	{
		if ((w > 0) && (flows[flow_idx]->pkts_in_q > 0))
		{
			w--;
		}
		else
		{
			flow_idx = nxt_prim_flow_idx(flows, flow_idx + 1);
			if (flow_idx < 0)
				return -1;

			w = flows[flow_idx]->weight - 1;
		}
	}

	return flow_idx;
}


/**
  ***************************************************************************
  * brief:  schedule next flow using WDRR algorithem
  * input:  flows array
  * output: idx of scheduled flow, -1 if none found
  ***************************************************************************
  */
int sch_WDRR(struct flow *flows[])
{
	static int last_flow_idx = -1;
	static int flow_idx = -1;
	struct flow *flow;

	// no flow has dibs
	if (flow_idx < 0)
	{
		flow_idx = nxt_prim_flow_idx(flows, 0);
		if (flow_idx < 0)
			return -1;
	}

	// need to schedule new packet
	if (g_tx <= 1)
	{
		while (1)
		{
			flow = flows[flow_idx];
			
			// flow has packets
			if (flow->pkts_in_q > 0)
			{
				// give flow credit (only pre-tx)
				if (flow_idx != last_flow_idx)
					flow->credit += flows[flow_idx]->weight;
				
				short pkt_len = flow->pkt_q.tqh_first->length;
				
				// flow has enough credit
				if (pkt_len <= flow->credit)
				{
					flow->credit -= pkt_len;
					last_flow_idx = flow_idx;
					break;
				}
				// else choose next flow
				else
				{
					last_flow_idx = -1;
					flow_idx = nxt_prim_flow_idx(flows, flow_idx + 1);
					if (flow_idx < 0)
						return -1;

					continue;
				}
			}
			// flow has no packets
			else
			{
				flow->credit = 0;

				// choose new flow
				flow_idx = nxt_prim_flow_idx(flows, flow_idx + 1);
				if (flow_idx < 0)
					return -1;

				continue;
			}
		}
	}

	return flow_idx;
}


// scheduler functions array
int(*sch[SCH_MAX])(struct flow *flows[]) =
{
	[SCH_WRR]  = sch_WRR,
	[SCH_WDRR] = sch_WDRR
};


/**
  ***************************************************************************
  * brief:  "tx" a packet into output file
  * input:  output file, flows array, idx of flow that has dibs
  ***************************************************************************
  */
void tx(FILE *fp, struct flow *flows[], int flow_idx)
{
	static bool first_tx = true;

	// for stats
	if (first_tx)
	{
		g_stat_time = g_time;
		first_tx = false;
	}
		
	// ongoing tx OR nothing to tx
	if (--g_tx > 0 || flow_idx < 0)
		return;

	// tx packet
	struct flow *flow = flows[flow_idx];
	struct packet *tx_pkt = flow->pkt_q.tqh_first;
	g_tx = tx_pkt->length;
	fprintf(fp, "%li: %li\n", g_time, tx_pkt->packt_id);

	// gather delay stats
	long int delay = g_time - tx_pkt->arrival_time;
	flow->stats.acmltd_delay += delay;
	if (delay > flow->stats.max_delay)
		flow->stats.max_delay = delay;
	
	// remove packet
	pop_packet(flow);
}


/**
  ***************************************************************************
  * brief:  scans packet from input file
  * input:  input file pointer,  temp packet pointer, temp flow info pointer
  *			temp weight pointer, default weight
  * output: number of args read from file (expected 7, 8 or 0)
  ***************************************************************************
  */
int scan_packet(FILE *f, struct packet *pkt, struct flow_info *f_info, int *weight, int dflt_w)
{
	// scan from file to string
	char line[MAX_INPUT_LINE_LEN];
	fgets(line, MAX_INPUT_LINE_LEN, f);

	// scan string for data
	int num = sscanf(line, "%li %li %s %hu %s %hu %hu %i\n",
		&pkt->packt_id,
		&pkt->arrival_time,
		f_info->src_add,
		&f_info->src_port,
		f_info->dst_add,
		&f_info->dst_port,
		&pkt->length,
		weight);

	// check valid input
	CHECK_ERROR("scan_packet()", (num == 7 || num == 8 || num == 0), 0);
	
	// assign default weight
	if (num == 7)
		*weight = dflt_w;

	return num;
}


/**
  ***************************************************************************
  * brief:  allocate memory for new packet and push as tail to flow queue
  * input:  flow pointer, packet pointer
  ***************************************************************************
  */
void push_packet(struct flow *flow, struct packet *pkt)
{	
	if (flow->pkts_in_q == 0)
	{
		TAILQ_INSERT_HEAD(&flow->pkt_q, pkt, q);
	}
	else
	{
		TAILQ_INSERT_TAIL(&flow->pkt_q, pkt, q);
	}

	pkt->flow = flow;
	flow->pkts_in_q++;
	flow->stats.num_packets++;
}


/**
  ***************************************************************************
  * brief:  pops head packet from flow queue, and frees packet memory
  * input:  flow pointer
  ***************************************************************************
  */
void pop_packet(struct flow *flow)
{
	struct packet *pkt = flow->pkt_q.tqh_first;
	CHECK_ERROR("pop_packet()", pkt, NULL);

	// remove packet
	TAILQ_REMOVE(&flow->pkt_q, pkt, q);
	flow->pkts_in_q--;
	free(pkt);
}


/**
  ***************************************************************************
  * brief:  initialize a new flow
  * input:  flows array, free idx in flows array, info of flow, weight of flow
  ***************************************************************************
  */
void init_flow(struct flow *flows[], int idx, struct flow_info *info, int weight)
{	
	// set flow
	flows[idx] = calloc(1, sizeof(struct flow));
	memcpy(&flows[idx]->info, info, sizeof(struct flow_info));
	flows[idx]->weight = weight;

	// init buffer queue
	TAILQ_INIT(&flows[idx]->pkt_q);
}


/**
  ***************************************************************************
  * brief:  update buffer stats
  * input:  flows array
  ***************************************************************************
  */
void stat_buff(struct flow *flows[])
{
	for (int i = 0; flows[i]; ++i)
	{
		int buff_size = flows[i]->pkts_in_q;
		flows[i]->stats.acmltd_buff += buff_size;
		if (buff_size > flows[i]->stats.max_buff)
			flows[i]->stats.max_buff = buff_size;
	}
}
