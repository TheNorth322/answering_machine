#include "../headers/answering_machine.h"
#include "glib.h"
#include <pj/config.h>
#include <pj/types.h>
#include <pjsua-lib/pjsua.h>

/*
 * on_incoming_call - callback used when call is
 * received.
 */
static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata) {
  pjsua_call_info ci;
  pj_timer_entry* timer;
  
  PJ_UNUSED_ARG(acc_id);
  PJ_UNUSED_ARG(rdata);
  
  pjsua_call_get_info(call_id, &ci);
  
  /* Ringing state */
  pjsua_call_answer(call_id, 180, NULL, NULL); 

  /* TODO: Run timer for 1 sec */
  
  char* uri = ci.remote_info.ptr;
  
  unsigned int result = lookup_uri(table, uri); 
  
  /* Not found in table, send Forbidden response */
  if (result == 0) {
    pjsua_call_hangup(call_id, 403, NULL, NULL); 
  }
  
  /* Send OK response */
  pjsua_call_answer(call_id, 200, NULL, NULL);
}


/*
 * on_call_state - callback used when call state
 * is changed.
 */
static void on_call_state(pjsua_call_id call_id, pjsip_event *e) {
  pjsua_call_info ci;

  PJ_UNUSED_ARG(e);
  
  pjsua_call_get_info(call_id, &ci);
  
  /* TODO: Timer for 1 sec */

  /* TODO: Log */
}

/*
 * on_call_media_state - callback used when call
 * media state is changed.
 */
static void on_call_media_state(pjsua_call_id call_id) {
  pjsua_call_info ci;
  
  pjsua_call_get_info(call_id, &ci);
   
  /* TODO: Connect call port ot p_slot of tone in conf bridge */
}

/*
 * init_pjsua_cb - used to initialize callbacks
 * in pjsua config.
 * @cfg - pointer to an object of pjsua_config 
 */
void init_pjsua_cb(pjsua_config* cfg) {
  cfg->cb.on_incoming_call = &on_incoming_call;
  cfg->cb.on_call_state = &on_call_state;
  cfg->cb.on_call_media_state = &on_call_media_state;
  cfg->max_calls = 100;
}

/*
 * init_pjsua_logging - used to initialize logging
 * in pjsua config.
 * @log_cfg - pointer to an object of pjsua_logging_config
 */
void init_pjsua_logging(pjsua_logging_config* log_cfg, int console_level) {
  log_cfg->console_level = 4;
}

/*
 * init_pjsua - used to initialize configs
 * for pjsua and init pjsua itself.
 */
void init_pjsua(void) {
  pj_status_t status;
  pjsua_config cfg;
  pjsua_logging_config log_cfg;
  
  /* Init callbacks */
  pjsua_config_default(&cfg);
  
  init_pjsua_cb(&cfg);

  /* Init logging */
  pjsua_logging_config_default(&log_cfg);
  init_pjsua_logging(&log_cfg, CONSOLE_LEVEL);

  status = pjsua_init(&cfg, &log_cfg, NULL);
  if (status != PJ_SUCCESS) {
    err_exit("Error in init of pjsua", status);
  }
}

/*
 * init_transport_proto - used to initialize
 * transport layer protocol to pjsua.
 */
void init_transport_proto(void) {
  pj_status_t status;
  pjsua_transport_config cfg;

  pjsua_transport_config_default(&cfg);
  cfg.port = PORT;

  status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &cfg, NULL);
  if (status != PJ_SUCCESS) {
    err_exit("Error creating transport cfg", status);
  }
}

/*
 * register_pjsua - used to add creadentials
 * to pjsua.
 */
pjsua_acc_id register_pjsua(void) {
  pj_status_t status;
  pjsua_acc_config cfg; 
  pjsua_acc_id acc_id;

  pjsua_acc_config_default(&cfg);
  cfg.id = pj_str("sip:" SIP_USER "@" SIP_DOMAIN);
  cfg.reg_uri = pj_str("sip:" SIP_DOMAIN);
  cfg.cred_count = 1;
  cfg.cred_info[0].realm = pj_str(SIP_DOMAIN);
  cfg.cred_info[0].scheme = pj_str("digest");
  cfg.cred_info[0].username = pj_str(SIP_USER);
  cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
  cfg.cred_info[0].data = pj_str(SIP_PASSWORD);
  
  status = pjsua_acc_add(&cfg, PJ_TRUE, &acc_id);

  return acc_id;
}

/*
 * recv_calls - used to recv calls from other
 * peers.
 */
void recv_calls(void) {
  while (1) {
    char option[10];

    puts("Press 'q' to quit");
    if (fgets(option, sizeof(option), stdin) == NULL) {
      puts("EOF while reading stdin, will quit not..");
    }

    if (option[0] == 'q') {
      break;
    }
  } 
}

unsigned int lookup_uri(GHashTable* table, char uri[MAX_URI]) {
  unsigned int* p_slot = g_hash_table_lookup(table, uri);
  
  /* URI not found */
  if (p_slot == NULL) {
    return 0;
  }

  return *p_slot;
}
