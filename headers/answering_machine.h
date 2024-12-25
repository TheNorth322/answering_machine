#ifndef ANSWERING_MACHINE_H 
#define ANSWERING_MACHINE_H

#include <pj/types.h>
#include <pjmedia/conference.h>
#include <pjmedia/types.h>
#include <pjsua-lib/pjsua.h>
#include <pj/hash.h>
#include "common.h"

#define SIP_DOMAIN ""
#define SIP_USER ""
#define SIP_PASSWORD ""
#define PORT 60000 
#define CONSOLE_LEVEL 4

struct answering_machine {
  pj_caching_pool cp;
  pj_pool_t* pool;
  pj_hash_table_t* table;
  pjmedia_conf* conf_bridge; 
  pjmedia_endpt* endpoint;
  pjmedia_port** ports;
  
  int ports_count;
  int ports_size; 
};

struct answering_machine* create_answering_machine(); 

static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata);

static void on_call_state(pjsua_call_id call_id, pjsip_event *e);

static void on_call_media_state(pjsua_call_id call_id);

void init_pjsua(void); 

void init_pools(void);

void init_conf_bridge(void);

void init_players(void);

void init_transport_proto(void);

pjsua_acc_id register_pjsua(void);

void recv_calls(void);

void add_port(pjmedia_port* port);

void free_answering_machine(void);

#endif // !ANSWERING_MACHINE_H 
