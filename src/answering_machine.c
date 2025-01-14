#include "../headers/answering_machine.h"
#include <pjsua-lib/pjsua.h>

/* App context */
struct answering_machine* machine;

/*
 * on_incoming_call - callback used when call is
 * received.
 */
static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata) {
  pjsua_call_info ci;
  pj_timer_entry* timer;
  pjsip_uri* generic_uri;
  pj_str_t username; 
  pjsua_conf_port_id* conf_port;  

  PJ_UNUSED_ARG(acc_id);
  PJ_UNUSED_ARG(rdata);
  
  pjsua_call_get_info(call_id, &ci);
  
  PJ_LOG(3,(THIS_FILE, "Incoming call to %.*s!!",
                        (int)ci.local_info.slen,
                        ci.local_info.ptr));
  
  struct call* call = create_call(call_id);
  add_call(call); 

  /* Get generic URI */
  generic_uri = pjsip_parse_uri(machine->pool, ci.local_info.ptr, ci.local_info.slen, 0);
  
  /* Extract username */
  username = extract_username(generic_uri);
  
  conf_port = pj_hash_get(machine->table, username.ptr, username.slen, 0);
  /* Hangup if username is not in the collection */
  if (conf_port == NULL) {
    pjsua_call_hangup(call_id, 403, NULL, NULL); 
    return;
  } 
  
  /* Ringing state */
  pjsua_call_answer(call_id, 180, NULL, NULL);
  
  PJ_LOG(3,(THIS_FILE, "Scheduling timer in %d", call_id));
  
  pj_timer_entry_init(call->ringing_timer, call_id, NULL, on_ringing_timer_callback); 
  pj_timer_entry_init(call->media_session_timer, call_id, NULL, on_media_state_timer_callback);

  call->ringing_timer->user_data = &call->call_id;   
  pjsua_schedule_timer(call->ringing_timer, &call->ringing_time);
}

/*
 * on_call_state - callback used when call state
 * is changed.
 */
static void on_call_state(pjsua_call_id call_id, pjsip_event *e) {
  pjsua_call_info ci;
  int* hash_table_value;
  struct call* call; 

  PJ_UNUSED_ARG(e);
  
  pjsua_call_get_info(call_id, &ci);

  PJ_LOG(3,(THIS_FILE, "Call %d state=%.*s", call_id,
                        (int)ci.state_text.slen,
                        ci.state_text.ptr));
  switch (ci.state) {
    case PJSIP_INV_STATE_CONFIRMED:
      PJ_LOG(3,(THIS_FILE, "Scheduling timer in %d", call_id));
      
      call = find_call(call_id);
      if (call == NULL) {
        perror("call is NULL");
        exit(EXIT_FAILURE);
      }

      /* Schedule timer for call */
      call->media_session_timer->user_data = &call->call_id;   
      pjsua_schedule_timer(call->media_session_timer, &call->media_time);
      break;
    case PJSIP_INV_STATE_DISCONNECTED:
      PJ_LOG(3, (THIS_FILE, "Deleting call %d", call_id));
      
      call = find_call(call_id);
      if (call == NULL) {
        break;
      }
      
      if (pj_timer_entry_running(call->media_session_timer) == PJ_TRUE) {
        pjsua_cancel_timer(call->media_session_timer);
      }
      
      /* Disconnect ports if connected */
      if (call->conf_port >= 0 && call->media_port >= 0) {
        pjsua_conf_disconnect(call->media_port, call->conf_port);
        pjsua_conf_remove_port(call->conf_port);
      }
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
  pjsua_conf_port_id media_port;
  pjsip_uri* generic_uri;
  pj_str_t username; 
  pjsua_call_info ci;
  struct call* call;

  pjsua_call_get_info(call_id, &ci);
  
  call = find_call(call_id);
  if (!call) {
    perror("call not found");
    exit(EXIT_FAILURE);
  }

  /* Get generic URI */
  generic_uri = pjsip_parse_uri(machine->pool, ci.local_info.ptr, ci.local_info.slen, 0);
  
  /* Extract username */
  username = extract_username(generic_uri);
 
  media_port = *(pjsua_conf_port_id*) pj_hash_get(machine->table, username.ptr, username.slen, 0);
  if (!media_port) { 
    PJ_LOG(3,(THIS_FILE, "Error: Port not found for %s", username.ptr)); 
    return; 
  } 
  
  conf_port = pjsua_call_get_conf_port(call_id); 
  
  /* Connect signal port and call port */
  pjsua_conf_connect(media_port, conf_port);
  
  call->conf_port = conf_port;
  call->media_port = media_port;

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

  /* TODO: Change response code */
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
  cfg->max_calls = 25;
}

/*
 * create_answering_machine - used to create and initialize 
 * answering machine.
 * Return: if successful returns pj_pool_t to allocate players 
 */
pj_pool_t* create_answering_machine(void) {
  pj_status_t status;
  pjsua_config cfg;
  pjsua_logging_config log_cfg;
  pjsua_media_config med_cfg;
  
  status = pjsua_create();
  if (status != PJ_SUCCESS) {
    err_exit("Error in pjsua_create()", status);
  }

  machine = (struct answering_machine*) malloc(sizeof(struct answering_machine));
  if (!machine) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  machine->players = (struct media_player**) malloc(PORTS * sizeof(struct media_player*));
  if (!machine->players) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  machine->calls = (struct call**) malloc(CALLS * sizeof(struct call*));
  if (!machine->calls) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
   
  machine->players_count = 0;
  machine->players_size = PORTS;
  
  machine->calls_count = 0;
  machine->calls_size = CALLS;

  /* Init callbacks */
  pjsua_config_default(&cfg);
  
  init_pjsua_cb(&cfg);

  /* Init logging */
  pjsua_logging_config_default(&log_cfg);
  log_cfg.console_level = CONSOLE_LEVEL;

  pjsua_media_config_default(&med_cfg);
  med_cfg.no_vad = PJ_TRUE;
  
  status = pjsua_init(&cfg, &log_cfg, &med_cfg);
  if (status != PJ_SUCCESS) {
    err_exit("Error in init of pjsua", status);
  }

  init_pools();

  init_transport_proto();

  machine->table = pj_hash_create(machine->pool, 1000);

  return machine->pool;
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
 * add_player - used to add player to players
 * collection.
 * @port - media port of player
 * @username - phone number to play sound on
 */
void add_player(pjmedia_port* port, const char* username) {
  pj_status_t status;
  pjsua_conf_port_id p_slot; 
  struct media_player* player = (struct media_player*) malloc(sizeof(struct media_player));

  /* Add media ports to conf bridge */
  pjsua_conf_add_port(machine->pool, port,  &p_slot);
  
  player->media_port = port; 
  player->conf_port = p_slot;

  /* Fill in the table with URI -> port_slots in conf bridge */
  pj_hash_set(machine->pool, machine->table, username, PJ_HASH_KEY_STRING, 0, &player->conf_port);
  
  /* Add ports to array */
  add_media_player(player);
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
void start_answering_machine(void) {
  pj_status_t status;

  /* Start pjsua */
  status = pjsua_start();  
  if (status != PJ_SUCCESS) {
    err_exit("Error starting pjsua", status);
  }
  
  machine->acc_id = register_pjsua();

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
void add_media_player(struct media_player* player) {
  int i = machine->players_count;

  if (machine->players_count >= machine->players_size) {
    return;
  } 

  machine->players[i] = player;
  machine->players_count++;
}

/*
 * extract_username - used to extract username
 * from generic uri. Gets scheme of the URI, casts 
 * generic URI to specific one and extracts username.
 * @generic_uri - URI itself
 *
 * Return: username which is string, if successful 
 */
pj_str_t extract_username(pjsip_uri* generic_uri) {
  const pj_str_t* schema;
  pjsip_sip_uri* sip_uri;
  
  /* Extract schema */
  schema = pjsip_uri_get_scheme(generic_uri);
  generic_uri = pjsip_uri_get_uri(generic_uri);

  /* Must have more cases if other addressing is used */
  /* SIP addressing  */
  if (strncmp(schema->ptr, "sip", schema->slen) == 0) {
    sip_uri = (pjsip_sip_uri*) generic_uri;
    return sip_uri->user;
  }
  // TODO: Add return value
}

/*
 * add_call - used to add call to calls collection.
 * @call - pointer to an object of call type 
 */
void add_call(struct call* call) {
  int i = machine->calls_count;

  if (machine->calls_count >= machine->calls_size) {
    return;
  }
  
  machine->calls[i] = call;
  machine->calls_count++;
}

/*
 * find_call - used to find call in collection
 * by call id.
 * @call_id - id of the call to find
 *
 * Return: call if successful, NULL otherwise
 */
struct call* find_call(pjsua_call_id call_id) {
  for (int i = 0; i < machine->calls_count; i++) {
    struct call* call = machine->calls[i]; 
    if (call->call_id == call_id)  {
      return call;
    }
  }

  return NULL;
}

/*
 * delete_call - used to delete call from the 
 * collection by call id. When call is deleted
 * shifts other calls to left by one.
 * @call_id - id of the call to delete 
 */
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
  for (int i = 0; i < machine->players_count; i++) {
    pjmedia_port_destroy(machine->players[i]->media_port);
    free(machine->players[i]);
  }
  free(machine->players);
  
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
