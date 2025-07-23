#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mongoose.h"
#include "sds.h"

#ifdef VERBOSE
#define LOG_VERBOSE(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define LOG_VERBOSE(fmt, ...)
#endif

struct ut_ws_conn {
	int index;
	sds responce;
	unsigned is_connected : 1;
	unsigned got_responce : 1;
};

void ev_handle(struct mg_connection* c, int ev, void* ev_data) {
	switch (ev) {
		case MG_EV_WS_OPEN:
			{
				LOG_VERBOSE("WS: OPEN");
				struct ut_ws_conn* utwsc = (struct ut_ws_conn*)c->fn_data;
				LOG_VERBOSE("INDEX: %d\n", utwsc->index);
				utwsc->is_connected = 1;
			}
			break;
		case MG_EV_WS_MSG:
			{
				LOG_VERBOSE("WS: MSG");
				struct ut_ws_conn* utwsc = (struct ut_ws_conn*)c->fn_data;
				LOG_VERBOSE("INDEX: %d\n", utwsc->index);
				struct mg_ws_message *wm = (struct mg_ws_message*)ev_data;
				LOG_VERBOSE("DATA: '%*.s'\n", wm->data.len, wm->data.buf);
				utwsc->responce = sdsnewlen(wm->data.buf, wm->data.len);
				utwsc->got_responce = 1;
			}
			break;
		case MG_EV_CLOSE:
			break;
	}
}

void ut_connect(struct mg_connection* c) {
	struct ut_ws_conn* utwsc = (struct ut_ws_conn*)c->fn_data;
	utwsc->is_connected = 0;
	while (!utwsc->is_connected) {
		mg_mgr_poll(c->mgr, 1000);
	}
	printf("CONNECTION %d OK\n", utwsc->index);
}

void ut_send(struct mg_connection* c, char* msg) {
	printf("SEND: %s\n");
	mg_ws_send(c, msg, strlen(msg), WEBSOCKET_OP_TEXT);
}

void ut_expect(struct mg_connection* c, char* res) {
	struct ut_ws_conn* utwsc = (struct ut_ws_conn*)c->fn_data;
	utwsc->got_responce = 0;
	while (!utwsc->got_responce) {
		mg_mgr_poll(c->mgr, 1000);
	}
	printf("RESPONCE: '%s'\n", utwsc->responce);
	sdsfree(utwsc->responce);
}

int main() {
	printf("BACKEND UNIT-TESTS:\n");
	LOG_VERBOSE("TEST VERBOSE\n");
	printf("Initializing WS connections...\n");
	struct mg_mgr mgr;
	struct mg_connection* c[16];
	struct ut_ws_conn utwsc[16] = {0};
	mg_mgr_init(&mgr);
	for (int i = 0; i < 16; i++) {
		utwsc[i].index = i;
		printf("Connection %d... \n");
		c[i] = mg_ws_connect(&mgr, "http://localhost:6969/ws", ev_handle, &utwsc[i], NULL);
	}
	printf("Begin testing...\n");
	ut_connect(c[0]);
	ut_send(c[0], "c_open|Coolname666");
	ut_expect(c[0], "c_open_ok");
	ut_expect(c[0], "c_users|Coolname666");
	return 0;
}
