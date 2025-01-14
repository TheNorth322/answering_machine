#ifndef CALL_H
#define CALL_H

#include <pjsua-lib/pjsua.h>

struct call {
  /* Ports in Conference Bridge */
  pjsua_conf_port_id conf_port;
  pjsua_conf_port_id media_port;
  
  /* Timers */
  pj_timer_entry* ringing_timer; 
  pj_timer_entry* media_session_timer;

  pjsua_call_id call_id;
};

struct call* create_call(pjsua_call_id call_id); 

int compare_calls(void* value, struct call* call);

#endif // !CALL_H
