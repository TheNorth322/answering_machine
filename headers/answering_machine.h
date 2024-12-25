#ifndef ANSWERING_MACHINE_H 
#define ANSWERING_MACHINE_H

#include <pj/types.h>
#include <pjsua-lib/pjsua.h>
#include "common.h"

#define SIP_DOMAIN ""
#define SIP_USER ""
#define SIP_PASSWORD ""
#define PORT 60000 
#define CONSOLE_LEVEL 4

static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata);

static void on_call_state(pjsua_call_id call_id, pjsip_event *e);

static void on_call_media_state(pjsua_call_id call_id);

void init_pjsua(void); 

void init_transport_proto(void);

pjsua_acc_id register_pjsua(void);

void recv_calls(void);

#endif // !ANSWERING_MACHINE_H 
