#ifndef CALL_H
#define CALL_H

#include <pjsua-lib/pjsua.h>

struct call {
  pjsua_conf_port_id conf_port;
  pjsua_conf_port_id sink_port;
  pjsua_call_id call_id;
};

struct call* create_call(pjsua_call_id call_id); 

int compare_calls(void* value, struct call* call);

#endif // !CALL_H
