#ifndef ANSWERING_MACHINE_H 
#define ANSWERING_MACHINE_H

#include <pj/timer.h>
#include <time.h>
#include <pj/types.h>
#include <pj/hash.h>
#include <pj/config.h>
#include <pj/hash.h>
#include <pj/types.h>
#include <pjmedia/conference.h>
#include <pjmedia/types.h>
#include <pjmedia/conference.h>
#include <pjmedia/port.h>
#include <pjsua-lib/pjsua.h>
#include <string.h>

#include "common.h"
#include "call.h"
#include "media_player.h"

#define SIP_DOMAIN    "10.25.72.25"
#define SIP_USER      "answerer"
#define SIP_PASSWORD  "password"
#define PORT          6222
#define CONSOLE_LEVEL 10

struct answering_machine {
  /* Pools */
  pj_caching_pool cp;
  pj_pool_t* pool;
  
  /* Calls */
  struct call** calls;  

  /* Table for URI -> p_slot map */
  pj_hash_table_t* table;

  /* Media */
  pjmedia_conf* conf_bridge; 
  pjmedia_endpt* endpoint;
  
  struct media_player** players;
  
  pjsua_acc_id acc_id;

  int players_count;
  int players_size; 

  int calls_count;
  int calls_size;
};

pj_pool_t* create_answering_machine(void);

static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata);

static void on_call_state(pjsua_call_id call_id, pjsip_event *e);

static void on_call_media_state(pjsua_call_id call_id);

static void on_ringing_timer_callback(pj_timer_heap_t* timer_heap, struct pj_timer_entry *entry);

static void on_media_state_timer_callback(pj_timer_heap_t* timer_heap, struct pj_timer_entry *entry);

void init_pools(void);

void add_player(pjmedia_port* port, const char* username);

void init_transport_proto(void);

pjsua_acc_id register_pjsua(void);

void start_answering_machine(void);

pj_str_t extract_username(pjsip_uri* generic_uri);

void add_media_player(struct media_player* player);

void add_call(struct call* call);

struct call* find_call(pjsua_call_id call_id);

void delete_call(pjsua_call_id call_id);

void free_answering_machine(void);

#endif // !ANSWERING_MACHINE_H 
