#include "../headers/answering_machine.h"
#include <pj/types.h>
#include <pjsip-ua/sip_inv.h>
#include <pjsua-lib/pjsua.h>
#include <stdio.h>
#include <time.h>

/* App context */
struct answering_machine* machine;
pjsua_conf_port_id long_tone_p_slot;
pjsua_conf_port_id wav_p_slot;
pjsua_conf_port_id rbt_p_slot;

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
  
  PJ_LOG(3,(THIS_FILE, "Incoming call from %.*s!!",
                        (int)ci.remote_info.slen,
                        ci.remote_info.ptr));
  
  struct call* call = create_call(call_id);
  add_call(call); 

  /* Ringing state */
  pjsua_call_answer(call_id, 180, NULL, NULL);
  
  PJ_LOG(3,(THIS_FILE, "Scheduling timer in %d", call_id));
  machine->ringing_timer->user_data = &call->call_id;   
  pjsua_schedule_timer(machine->ringing_timer, &machine->ringing_time);
}

/*
 * on_call_state - callback used when call state
 * is changed.
 */
static void on_call_state(pjsua_call_id call_id, pjsip_event *e) {
  pjsua_call_info ci;
  int* hash_table_value;

  PJ_UNUSED_ARG(e);
  
  pjsua_call_get_info(call_id, &ci);

  PJ_LOG(3,(THIS_FILE, "Call %d state=%.*s", call_id,
                        (int)ci.state_text.slen,
                        ci.state_text.ptr));

  switch (ci.state) {
    case PJSIP_INV_STATE_CONFIRMED:
      PJ_LOG(3,(THIS_FILE, "Scheduling timer in %d", call_id));
      
      struct call* call = find_call(call_id);
      
      if (call == NULL) {
        perror("call is NULL");
        exit(EXIT_FAILURE);
      }

      /* Schedule timer for call */
      machine->media_session_timer->user_data = &call->call_id;   
      pjsua_schedule_timer(machine->media_session_timer, &machine->media_time);
      break;
    case PJSIP_INV_STATE_DISCONNECTED:
      PJ_LOG(3, (THIS_FILE, "Deleting call %d", call_id));
      delete_call(call_id); 
      break;
    default:
      break;
  }
}

/*
 * on_call_media_state - callback used when call
 * media state is changed.
 */
static void on_call_media_state(pjsua_call_id call_id) {
  pjsua_conf_port_id conf_port;
  pjsua_conf_port_id sink_port;
  pjsua_call_info ci;
  char* uri;
  int* hash_table_value;

  pjsua_call_get_info(call_id, &ci);
  
  uri = ci.remote_info.ptr;
  
  hash_table_value = pj_hash_get(machine->table, uri, PJ_HASH_KEY_STRING, 0); 
  if (!hash_table_value) { 
    PJ_LOG(3,(THIS_FILE, "Error: Port not found for %s", uri)); 
    return; 
  } 
    
  conf_port = pjsua_call_get_conf_port(call_id); 
  
  sink_port = *((int*) hash_table_value);

  /* Connect signal port and call port */
  pjsua_conf_connect(sink_port, conf_port);

  PJ_LOG(3,(THIS_FILE, "Media changed in %d", call_id));
}

/*
 * on_ringing_timer_callback - used on expiration of scheduled timer 
 * on INVITE request, after sending 180 Ringing.
 */
static void on_ringing_timer_callback(pj_timer_heap_t* timer_heap, struct pj_timer_entry *entry) {
  pjsua_call_info ci;
  pjsua_call_id* call_id = (pjsua_call_id*) entry->user_data;
  
  PJ_UNUSED_ARG(timer_heap);

  pjsua_call_get_info(*call_id, &ci);

  PJ_LOG(3,(THIS_FILE, "Call %d call timer expired", *call_id));
                         
  char* uri = ci.remote_info.ptr;
  
  void* slot_pointer = pj_hash_get(machine->table, uri, PJ_HASH_KEY_STRING, 0); 
  
  /* Not found in table, send Forbidden response */
  if (slot_pointer == NULL) {
    pjsua_call_hangup(*call_id, 403, NULL, NULL); 
  }
  
  /* Send OK response */
  pjsua_call_answer(*call_id, 200, NULL, NULL);
}

/*
 * on_media_state_timer_callback - used on expiration of timer scheduled
 * after call is answered.
 */
static void on_media_state_timer_callback(pj_timer_heap_t* timer_heap, struct pj_timer_entry *entry) {
  pjsua_call_id* call_id = (pjsua_call_id*) entry->user_data;
  
  PJ_UNUSED_ARG(timer_heap);

  /* TODO: Change code */
  pjsua_call_hangup(*call_id, 403, NULL, NULL); 
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
  log_cfg->console_level = 10;
}

/*
 * init_pjsua - used to initialize configs
 * for pjsua and init pjsua itself.
 */
void init_pjsua(void) {
  pj_status_t status;
  pjsua_config cfg;
  pjsua_logging_config log_cfg;
  pjsua_media_config med_cfg;
  
  machine = (struct answering_machine*) malloc(sizeof(struct answering_machine));
  machine->ports = (struct pjmedia_port**) malloc(PORTS * sizeof(struct pjmedia_port*));
  machine->calls = (struct call**) malloc(CALLS * sizeof(struct call*));
   
  machine->ports_count = 0;
  machine->ports_size = PORTS;
  
  machine->calls_count = 0;
  machine->calls_size = CALLS;

  /* Init callbacks */
  pjsua_config_default(&cfg);
  
  init_pjsua_cb(&cfg);

  /* Init logging */
  pjsua_logging_config_default(&log_cfg);
  init_pjsua_logging(&log_cfg, CONSOLE_LEVEL);
  
  pjsua_media_config_default(&med_cfg);
  med_cfg.no_vad = PJ_TRUE;
  
  status = pjsua_init(&cfg, &log_cfg, &med_cfg);
  if (status != PJ_SUCCESS) {
    err_exit("Error in init of pjsua", status);
  }
}

/*
 * init_pools - used to initialize memory pools
 * for pjsua.
 */
void init_pools(void) {
  /* Create pool for memory alloc */ 
  pj_caching_pool_init(&machine->cp, &pj_pool_factory_default_policy, 0);
  
  /* Create pool for media */   
  machine->pool = pj_pool_create(&machine->cp.factory, THIS_FILE, 4000, 4000, NULL);
  
  if (machine->pool == NULL) {
    perror("pj_pool_create");
    exit(EXIT_FAILURE);
  }
}

/*
 * init_conf_bridge - used to initialize conference
 * bridge.
 */
void init_conf_bridge(void) {
  pj_status_t status;
  
  /* Create media endpoint */
  status = pjmedia_endpt_create(&machine->cp.factory, NULL, 1, &machine->endpoint);
  if (status != PJ_SUCCESS) {
    err_exit("Error creating endpoint", status);
  }
      
  status = pjmedia_conf_create(machine->pool,
                               PORT_COUNT,
                               CLOCK_RATE,
                               NCHANNELS,
                               NSAMPLES,
                               NBITS,
                               0,
                               &machine->conf_bridge
                               );

  if (status != PJ_SUCCESS) {
    err_exit("Error creating conference bridge", status);
  }
}

/*
 * init_players - used to initialize signals and wav
 * player and add them to conference bridge.
 */
void init_players(void) {
  pj_status_t status;
  
  pjmedia_port* long_tone_port;
  pjmedia_port* wav_port;
  pjmedia_port* rbt_port;
     
  machine->table = pj_hash_create(machine->pool, 1000);

  /* Create long tonegen */
  status = pjmedia_tonegen_create(machine->pool, 8000, 1, 160, 16, PJMEDIA_TONEGEN_LOOP, &long_tone_port);
  if (status != PJ_SUCCESS) {
    err_exit("Error creating tonegen", status);
  }
  
  /* Init long tone */
  {
    pjmedia_tone_desc tones[1];

    tones[0].freq1 = LONG_TONE_FREQUENCY;
    tones[0].freq2 = 0;
    tones[0].on_msec = -1;
    tones[0].off_msec = 0;
    tones[0].volume = PJMEDIA_TONEGEN_VOLUME;
    
    status = pjmedia_tonegen_play(long_tone_port, 1, tones, 0);
    if (status != PJ_SUCCESS) {
      err_exit("Erroc playing tonegen", status);
    }
  }
  
  /* Create wav player */
  status = pjmedia_wav_player_port_create(machine->pool, WAV_FILE, PTIME, 0, 0, &wav_port);
  if (status != PJ_SUCCESS) {
    err_exit("Error in creating wav player", status);
  }

  /* Create rbt tonegen */
  status = pjmedia_tonegen_create(machine->pool, 8000, CHANNEL_COUNT, 64, 16, PJMEDIA_TONEGEN_LOOP, &rbt_port);
  if (status != PJ_SUCCESS) {
    err_exit("Error creating tonegen", status);
  }
  
  /* Init RBT tone */
  {
    pjmedia_tone_desc tones[1];

    tones[0].freq1 = LONG_TONE_FREQUENCY;
    tones[0].freq2 = 0;
    tones[0].on_msec = RBT_ON_MSEC;
    tones[0].off_msec = RBT_OFF_MSEC;
    tones[0].volume = PJMEDIA_TONEGEN_VOLUME;
    
    status = pjmedia_tonegen_play(rbt_port, 1, tones, 0);
    if (status != PJ_SUCCESS) {
      err_exit("Erroc playing tonegen", status);
    }
  }
  
  /* Add media ports to conf bridge */
  pjsua_conf_add_port(machine->pool, long_tone_port,  &long_tone_p_slot);
  pjsua_conf_add_port(machine->pool, wav_port, &wav_p_slot);
  pjsua_conf_add_port(machine->pool, rbt_port, &rbt_p_slot);
  
  /* Fill in the table with URI -> port_slots in conf bridge */
  pj_hash_set(machine->pool, machine->table, "<sip:danil@10.25.72.25>", PJ_HASH_KEY_STRING, 0, &long_tone_p_slot);
  pj_hash_set(machine->pool, machine->table, "<sip:danil1@10.25.72.25>", PJ_HASH_KEY_STRING, 0, &wav_p_slot);
  pj_hash_set(machine->pool, machine->table, "<sip:danil2@10.25.72.25>", PJ_HASH_KEY_STRING, 0, &rbt_p_slot);
  
  /* Add ports to array */
  add_port(long_tone_port);
  add_port(wav_port);
  add_port(rbt_port);
}

/*
 * init_timers - used to initialize timers
 * that are used in app.
 */
void init_timers(void) {
  machine->ringing_timer = (pj_timer_entry*) malloc(sizeof(struct pj_timer_entry)); 
  machine->media_session_timer = (pj_timer_entry*) malloc(sizeof(struct pj_timer_entry)); 
  
  machine->ringing_time.sec = RINGING_TIME;
  machine->ringing_time.msec = 0;

  machine->media_time.sec = MEDIA_SESSION_TIME;
  machine->media_time.msec = 0;

  pj_timer_entry_init(machine->ringing_timer, 1, NULL, on_ringing_timer_callback); 
  pj_timer_entry_init(machine->media_session_timer, 2, NULL, on_media_state_timer_callback); 
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
  cfg.id = pj_str("sip:" SIP_USER "@" SIP_DOMAIN "\0");
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

  free_answering_machine();
}

/*
 * add_port - used to add media port to an array
 * to free them later.
 */
void add_port(pjmedia_port* port) {
  int i = machine->ports_count;

  if (machine->ports_count >= machine->ports_size) {
    return;
  } 

  machine->ports[i] = port;
  machine->ports_count++;
}

void add_call(struct call* call) {
  int i = machine->calls_count;

  if (machine->calls_count >= machine->calls_size) {
    return;
  }
  
  machine->calls[i] = call;
  machine->calls_count++;
}

struct call* find_call(pjsua_call_id call_id) {
  for (int i = 0; i < machine->calls_count; i++) {
    struct call* call = machine->calls[i]; 
    if (call->call_id == call_id)  {
      return call;
    }
  }

  return NULL;
}

void delete_call(pjsua_call_id call_id) {
  int i;

  for (i = 0; i < machine->calls_count; i++) {
    struct call* call = machine->calls[i];
    if (call->call_id == call_id) {
      free(call);
      break;
    }
  }

  for (i = i; i < machine->calls_count; i++) {
    machine->calls[i] = machine->calls[i + 1];      
  }

  machine->calls_count--;
}
/*
 * free_answering_machine - used to free allocated memory
 * and destroy objects.
 */
void free_answering_machine(void) {
  /* Destroy media ports */
  for (int i = 0; i < machine->ports_count; i++) {
    pjmedia_port_destroy(machine->ports[i]);
  }
  free(machine->ports);
  
  /* Free calls */
  for (int i = 0; i < machine->calls_count; i++) {
    free(machine->calls[i]);
  }
  free(machine->calls);

  /* Release application pool */
  pj_pool_release(machine->pool);

  /* Destroy media endpoint. */
  pjmedia_endpt_destroy(machine->endpoint);
  
  /* Destroy conf bridge */
  pjmedia_conf_destroy(machine->conf_bridge);

  /* Destroy pool factory */
  pj_caching_pool_destroy(&machine->cp);
  
  pjsua_destroy();

  free(machine);
}
