#ifndef COMMON_H
#define COMMON_H

#define THIS_FILE "APP"

#define TIMER_ON_CALL 1000

#define CHANNEL_COUNT 1
#define PORT_COUNT 100
#define MAX_URI 16
#define PORTS 16

/* Constants */
#define CLOCK_RATE      44100
#define NSAMPLES        (CLOCK_RATE * 20 / 1000)
#define NCHANNELS       1
#define NBITS           16

/* First tone */
#define LONG_TONE_FREQUENCY 425

/* Second audio message */
#define WAV_FILE "example.wav" 
#define WAV_BITRATE 64000
#define WAV_FREQUENCY 8000
#define PTIME 20

/* Third tone */
#define RBT_FREQUENCY 425
#define RBT_ON_MSEC 1000
#define RBT_OFF_MSEC 4000

#define err_exit(title, status) do {\
  pjsua_perror(THIS_FILE, title, status);\
  pjsua_destroy();\
  exit(EXIT_FAILURE);} while(0)

#endif // !COMMON_H
