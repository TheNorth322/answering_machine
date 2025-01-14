#ifndef MEDIA_PLAYER_H
#define MEDIA_PLAYER_H

#include <pjmedia/port.h>
#include <pjsua-lib/pjsua.h>

struct media_player {
  pjsua_conf_port_id conf_port;
  pjmedia_port* media_port;
};

#endif // MEDIA_PLAYER
