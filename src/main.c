#include <pj/hash.h>
#include <pj/types.h>
#include <pjmedia/conference.h>
#include <pjmedia/config.h>
#include <pjmedia/endpoint.h>
#include <pjmedia/tonegen.h>
#include <pjsip/sip_transport_tls.h>
#include <stdlib.h>
#include <pjsua-lib/pjsua.h>

#include "../headers/answering_machine.h"

int main(void) {

  pj_status_t status;
  pj_caching_pool cp;
  pjmedia_endpt* med_endpt;
  pjmedia_port* long_tone_port;
  pjmedia_port* wav_port;
  pjmedia_port* rbt_port;
  pjmedia_conf* conf;
  pj_pool_t* pool;
  pj_hash_table_t* table; 
  unsigned int long_tone_p_slot;
  unsigned int wav_p_slot;
  unsigned int rbt_p_slot;

  status = pjsua_create();
  if (status != PJ_SUCCESS) {
    err_exit("Error in pjsua_create()", status);
  }
  
  init_pjsua();

  init_transport_proto();
  
  /* Create pool for memory alloc */ 
  pj_caching_pool_init(&cp, &pj_pool_factory_default_policy, 0);
  
  /* Create media endpoint */
  status = pjmedia_endpt_create(&cp.factory, NULL, 1, &med_endpt);
  if (status != PJ_SUCCESS) {
    err_exit("Error creating endpoint", status);
  }
  
  /* Create pool for media */   
  pool = pj_pool_create(&cp.factory, THIS_FILE, 4000, 4000, NULL);
  
  /* Create conference bridge */
  status = pjmedia_conf_create(pool,
                               PORT_COUNT,
                               CLOCK_RATE,
                               NCHANNELS,
                               NSAMPLES,
                               NBITS,
                               0,
                               &conf);
  if (status != PJ_SUCCESS) {
    err_exit("Error creating conference bridge", status);
  }
  
  table = pj_hash_create(pool, 1000);

  /* Create long tonegen */
  status = pjmedia_tonegen_create(pool, 8000, CHANNEL_COUNT, 64, 16, PJMEDIA_TONEGEN_LOOP, &long_tone_port);
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
  status = pjmedia_wav_player_port_create(pool, WAV_FILE, PTIME, 0, 0, &wav_port);
  if (status != PJ_SUCCESS) {
    err_exit("Error in creating wav player", status);
  }

  /* Create rbt tonegen */
  status = pjmedia_tonegen_create(pool, 8000, CHANNEL_COUNT, 64, 16, PJMEDIA_TONEGEN_LOOP, &rbt_port);
  if (status != PJ_SUCCESS) {
    err_exit("Error creating tonegen", status);
  }
  
  /* Init RBT tone */
  {
    pjmedia_tone_desc tones[1];
    tones[0].freq1 = RBT_FREQUENCY;
    tones[0].freq2 = 0;
    tones[0].on_msec = RBT_ON_MSEC;
    tones[0].off_msec = RBT_OFF_MSEC;
    tones[0].volume = PJMEDIA_TONEGEN_VOLUME; 

    status = pjmedia_tonegen_play(rbt_port, 1, tones, 0); 
    if (status != PJ_SUCCESS) {
      err_exit("Error playing RBT", status);
    }
  }
  
  /* Add media ports to conf bridge */
  pjmedia_conf_add_port(conf, pool, long_tone_port, NULL, &long_tone_p_slot);
  pjmedia_conf_add_port(conf, pool, wav_port, NULL, &wav_p_slot);
  pjmedia_conf_add_port(conf, pool, rbt_port, NULL, &rbt_p_slot);

  /* Fill in the table with URI -> port_slots in conf bridge */
  pj_hash_set(pool, table, "101", PJ_HASH_KEY_STRING, 0, &long_tone_p_slot);
  pj_hash_set(pool, table, "102", PJ_HASH_KEY_STRING, 0, &wav_p_slot);
  pj_hash_set(pool, table, "103", PJ_HASH_KEY_STRING, 0, &rbt_p_slot);
  
  /* Start pjsua */
  status = pjsua_start();  
  if (status != PJ_SUCCESS) {
    err_exit("Error starting pjsua", status);
  }
  
  register_pjsua();

  recv_calls();
  
  /* Delete ports */
  pjmedia_port_destroy(long_tone_port);

  pjmedia_port_destroy(rbt_port);

  /* Release application pool */
  pj_pool_release(pool);

  /* Destroy media endpoint. */
  pjmedia_endpt_destroy(med_endpt);

  /* Destroy pool factory */
  pj_caching_pool_destroy(&cp);
  
  pjsua_destroy();

  exit(EXIT_SUCCESS);
}
