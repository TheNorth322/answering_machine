#include "../headers/call.h"

struct call* create_call(pjsua_call_id call_id) {
  struct call* call = (struct call*) malloc(sizeof(struct call));
  
  call->call_id = call_id; 

  return call;
}

int compare_calls(void* value, struct call* call) {
  pjsua_call_id* target_id = (pjsua_call_id*) value;
  
  return *target_id == call->call_id ? 0 : 1;   
}
