#include "mongoose.h"

// V A R I A B L E S

//struct mg_connection* pl1 = NULL; // player1


// H A N D L E R S

// Messages from server:
// ...

//void send_all(struct mg_mgr *mgr, char* buf, size_t len) {
//	printf("Sending all: '%.*s'\n", len, buf);
//	struct mg_connection *c;
//	for (c = mgr->conns; c != NULL; c = c->next) {
//		if (c->is_websocket) {
//			mg_ws_send(c, buf, len, WEBSOCKET_OP_TEXT);
//		}
//	}
//}

// E V E N T S

void ev_handle_http_msg(struct mg_connection* c, void* ev_data) {
	struct mg_http_message* hm = (struct mg_http_message*)ev_data;
	if (mg_strcmp(hm->uri, mg_str("/ws")) == 0) {
		mg_ws_upgrade(c, hm, NULL);
		// TODO: Player connect
		printf("WS: New conn\n");
		return;
	}
	if (!strncmp(hm->method.buf, "GET", 3)) {
		struct mg_http_serve_opts opts = { .root_dir = "./web" };
		mg_http_serve_dir(c, hm, &opts);
	}
}

void ev_handler(struct mg_connection* c, int ev, void* ev_data) {
	switch (ev) {
		case MG_EV_HTTP_MSG:
			ev_handle_http_msg(c, ev_data);
			break;
		case MG_EV_WS_MSG:
			struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
			// TODO: WS message
			break;
		case MG_EV_CLOSE:
			// TODO: Player disconnect
			break;
	}
}

// M A I N

int main(void) {
	struct mg_mgr mgr;
	mg_mgr_init(&mgr);
	mg_http_listen(&mgr, "http://0.0.0.0:6969", ev_handler, NULL);
	for (;;) {
		mg_mgr_poll(&mgr, 1000);
	}
	return 0;
}
