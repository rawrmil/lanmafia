#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "mongoose.h"
#include "sds.h"

struct ws_conn_data{
	sds responce;
	unsigned is_connected : 1;
	unsigned got_responce : 1;
};

void ws_ev_handler(struct mg_connection* c, int ev, void* ev_data) {
	struct ws_conn_data* cd = (struct ws_conn_data*)c->fn_data;
	switch (ev) {
		case MG_EV_WS_OPEN:
			printf("WS_OPEN\n");
			cd->is_connected = 1;
			break;
		case MG_EV_WS_MSG:
			printf("WS_MSG\n");
			struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
			cd->got_responce = 1;
			cd->responce = sdsnewlen(wm->data.buf, wm->data.len);
			printf("DATA: '%s'\n", cd->responce);
			break;
		case MG_EV_CLOSE:
			printf("WS_CLOSE\n");
			sdsfree(cd->responce);
			free(cd);
			break;
		case MG_EV_ERROR:
			printf("WS_ERROR\n");
			// TODO: Log error
			exit(1);
			break;
	}
}

void ws_connect(struct mg_mgr* mgr, struct mg_connection** cp) {
	struct ws_conn_data* cd = calloc(1, sizeof(*cd));
	*cp = mg_ws_connect(mgr, "http://localhost:6969/ws", ws_ev_handler, cd, NULL);
	struct mg_connection* c = *cp;
	cd->is_connected = 0;
	for (int i = 0;; i++) {
		if (cd->is_connected)
			break;
		if (i == 3) {
			// TODO: Error
			exit(1);
		}
		mg_mgr_poll(c->mgr, 1000);
	}
}

void ws_send(struct mg_connection* c, char* msg) {
	printf("SEND:   '%s'\n", msg);
	mg_ws_send(c, msg, strlen(msg), WEBSOCKET_OP_TEXT);
}

void ws_expect(struct mg_connection* c, char* exp) {
	struct ws_conn_data* cd = (struct ws_conn_data*)c->fn_data;
	cd->got_responce = 0;
	for (int i = 0;; i++) {
		printf("POLL: %d\n", i);
		if (cd->got_responce)
			break;
		if (i == 3) {
			// TODO: Error
			exit(1);
		}
		mg_mgr_poll(c->mgr, 1000);
	}
	printf("EXPECT: '%s'\n", exp);
	printf("GOT:    '%s'\n", cd->responce);
	sdsfree(cd->responce);
}

void ws_disconnect(struct mg_connection* c) {
	c->is_closing = 1;
}

int main() {
	struct mg_mgr mgr;
	struct mg_connection* c[16] = {0};
	mg_mgr_init(&mgr);
	ws_connect(&mgr, &c[0]);
	ws_send(c[0], "c_open|Someguy1");
	ws_expect(c[0], "c_open_ok");
	ws_expect(c[0], "c_users|Someguy1");
}
