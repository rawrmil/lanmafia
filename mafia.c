#include "mongoose.h"
#include "unistr.h"
#include "sds.h"

// E V E N T S

// EV: Room connections
/*
WebSockets:
- Client message - U
- Server message - S
- U/S: '<type>|<data>'
- CONNECTING/DISCONNECTING:
-- U: 'rc_user_open|<name>' - open connection with name
-- S: 'rc_serv_open_ok' - connection successeful (send data to all players)
-- S: 'rc_serv_open_err|<error_id>' - connection unsuccesseful
-- <error_id>:
--- 'name_length' - name length > 32
--- 'name_exists' - name exists
--- 'conn_exists' - connection exists
-- U: 'rc_user_close' - close connection
-- U: '!|<data>' - send message directly to the game engine
*/

struct rc_conn {
	struct mg_str name;
	struct mg_connection* c;
	struct rc_conn* next;
};

struct rc_mgr {
	struct rc_conn* conns;
} rcmgr;

void rc_send_all(char* buf, int len) {
	struct rc_conn* rcc = rcmgr.conns;
	while (rcc != NULL) {
		mg_ws_send(rcc->c, buf, len, WEBSOCKET_OP_TEXT);
		rcc = rcc->next;
	}
}

#define WS_SEND_CONST(c_, arr_) mg_ws_send(c_, arr_, sizeof(arr_)-1, WEBSOCKET_OP_TEXT);

void rc_user_open(struct mg_connection* c, struct mg_str data) {
	if (data.len > 128 || u8_strlen(data.buf) > 32) {
		WS_SEND_CONST(c, "rc_serv_open_err|name_length");
		return;
	}
	if (u8_strstr(data.buf, ",") != NULL) {
		WS_SEND_CONST(c, "rc_serv_open_err|name_forbidden");
		return;
	}
	// Name/connection duplication check
	struct rc_conn* rcc = rcmgr.conns;
	while (rcc != NULL) {
		if (mg_strcmp(data, rcc->name) == 0) {
			WS_SEND_CONST(c, "rc_serv_open_err|name_exists");
			return;
		}
		if (c == rcc->c) {
			WS_SEND_CONST(c, "rc_serv_open_err|conn_exists");
			return;
		}
		rcc = rcc->next;
	}
	rcc = calloc(1, sizeof(*rcc));
	assert(rcc);
	rcc->name.buf = calloc(data.len, sizeof(char));
	assert(rcc->name.buf);
	rcc->name.len = data.len;
	strncpy(rcc->name.buf, data.buf, data.len);
	rcc->c = c;
	rcc->next = NULL;
	if (rcmgr.conns == NULL) {
		rcmgr.conns = rcc;
	} else {
		LIST_ADD_HEAD(struct rc_conn, &rcmgr.conns, rcc);
	}
	// TODO: free stuff
	sds usrstr = sdsnew("rc_serv_users|");
	while (rcc != NULL) {
		usrstr = sdscat(usrstr, rcc->name.buf);
		rcc = rcc->next;
		if (rcc != NULL)
			usrstr = sdscat(usrstr, ",");
	}
	rc_send_all(usrstr, sdslen(usrstr));
	sdsfree(usrstr);
	WS_SEND_CONST(c, "rc_serv_open_ok");
}

void rc_user_close(struct mg_connection* c, struct mg_str data) {
	struct rc_conn** rccp = &rcmgr.conns;
	while (*rccp != NULL) {
		if (c == (*rccp)->c) {
			free((*rccp)->name.buf);
			free((*rccp));
			*rccp = (*rccp)->next;
			WS_SEND_CONST(c, "rc_serv_close_ok");
			break;
		}
		rccp = &(*rccp)->next;
	}
}

// Game functions

// EV: Low level handlers

void ev_handle_http_msg(struct mg_connection* c, void* ev_data) {
	struct mg_http_message* hm = (struct mg_http_message*)ev_data;
	if (mg_strcmp(hm->uri, mg_str("/ws")) == 0) {
		mg_ws_upgrade(c, hm, NULL);
		printf("WS: New conn\n");
		return;
	}
	if (!strncmp(hm->method.buf, "GET", 3)) {
		struct mg_http_serve_opts opts = { .root_dir = "./web" };
		mg_http_serve_dir(c, hm, &opts);
	}
}


#define RC_PREFIX_CASE(_pre, _handler) \
	{ \
		if (sizeof(_pre)-1 <= wm->data.len && strncmp(wm->data.buf, _pre, sizeof(_pre)-1) == 0) { \
			if (sizeof(_pre)-1 == wm->data.len) \
				_handler(c, (struct mg_str) { .buf = NULL, .len = 0 }); \
			else \
				_handler(c, (struct mg_str) { .buf = wm->data.buf+sizeof(_pre), .len = wm->data.len-sizeof(_pre) }); \
			return; \
		} \
	}

void ev_handle_ws_msg(struct mg_connection* c, void* ev_data) {
	struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
	printf("'wm->data.buf': '%s'\n", wm->data.buf);
	RC_PREFIX_CASE("rc_user_open", rc_user_open);
	RC_PREFIX_CASE("rc_user_close", rc_user_close);
	//RC_PREFIX_CASE("test2", test2)
}

void ev_handle_ws_close(struct mg_connection* c, void* ev_data) {
	//struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
	//disconnect
	// TODO: Add rc_user_close
}

void ev_handler(struct mg_connection* c, int ev, void* ev_data) {

	switch (ev) {
		case MG_EV_HTTP_MSG:
			ev_handle_http_msg(c, ev_data);
			break;
		case MG_EV_WS_MSG:
			ev_handle_ws_msg(c, ev_data);
			break;
		case MG_EV_CLOSE:
			ev_handle_ws_close(c, ev_data);
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
