#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "config_manager.h"

void web_server_start(lora_config_t *config);
void web_server_stop(void);

#endif // WEB_SERVER_H