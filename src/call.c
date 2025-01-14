#include "../headers/call.h"

struct call* create_call(pjsua_call_id call_id) {
  struct call* call = (struct call*) malloc(sizeof(struct call));
  
  call->ringing_timer = (pj_timer_entry*) malloc(sizeof(struct pj_timer_entry)); 
  if (!call->ringing_timer) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  call->media_session_timer = (pj_timer_entry*) malloc(sizeof(struct pj_timer_entry)); 
  if (!call->media_session_timer) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  
  
  call->call_id = call_id; 
  call->conf_port = -1;
  call->media_port = -1;
  
  call->ringing_time.sec = RINGING_TIME;
  call->ringing_time.msec = 0;

  call->media_time.sec = MEDIA_SESSION_TIME;
  call->media_time.msec = 0;

  return call;
}

int compare_calls(void* value, struct call* call) {
  pjsua_call_id* target_id = (pjsua_call_id*) value;
  
  return *target_id == call->call_id ? 0 : 1;   
}
