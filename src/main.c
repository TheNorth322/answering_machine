#include "../headers/answering_machine.h"

int main(void) {
  pj_status_t status;
  pjsua_acc_id acc_id;
  
  status = pjsua_create();
  if (status != PJ_SUCCESS) {
    err_exit("Error in pjsua_create()", status);
  }
  
  init_pjsua();

  init_transport_proto();
  
  init_pools();

  init_conf_bridge();
  
  init_players();
  
  init_timers();

  /* Start pjsua */
  status = pjsua_start();  
  if (status != PJ_SUCCESS) {
    err_exit("Error starting pjsua", status);
  }
  
  acc_id = register_pjsua();
  
  recv_calls();
    
  exit(EXIT_SUCCESS);
}
