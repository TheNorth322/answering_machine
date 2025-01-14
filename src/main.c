#include "../headers/answering_machine.h"

int main(void) {
  pj_status_t status;
  
  pjmedia_port* long_tone_port;
  pjmedia_port* wav_port;
  pjmedia_port* rbt_port;

  pj_pool_t* pool;

  pool = create_answering_machine();

  /* Create long tonegen */
  status = pjmedia_tonegen_create(pool, 8000, 1, 160, 16, PJMEDIA_TONEGEN_LOOP, &long_tone_port);
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
  
  add_player(long_tone_port, "longtone");

  /* Create wav player */
  status = pjmedia_wav_player_port_create(pool, WAV_FILE, PTIME, 0, 0, &wav_port);
  if (status != PJ_SUCCESS) {
    err_exit("Error in creating wav player", status);
  }
  
  add_player(wav_port, "wav");

  /* Create rbt tonegen */
  status = pjmedia_tonegen_create(pool, 8000, CHANNEL_COUNT, 64, 16, PJMEDIA_TONEGEN_LOOP, &rbt_port);
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
  
  add_player(rbt_port, "rbt");
    
  start_answering_machine();
    
  exit(EXIT_SUCCESS);
}
