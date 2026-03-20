#ifndef ROUTER_H
#define ROUTER_H

#include "mongoose.h"

// 处理 HTTP 请求路由
void router_handle_request(struct mg_connection *c, struct mg_http_message *hm);
void router_broadcast_gpio_if_changed(struct mg_mgr *mgr);

#endif // ROUTER_H
