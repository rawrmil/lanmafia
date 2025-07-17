#include "mongoose.h"
#include "unistr.h"
#include "sds.h"

// E V E N T S

// EV: App connections
/*
Note: there is mongoose connections and app connections (don't confuse)
WebSockets:
	Client message - U
	Server message - S
	U/S: '<type>|<data>'
	CONNECTING/DISCONNECTING:
		U: 'c_open|<name>' - open user connection with name
		S: 'c_open_ok' - connection successeful (send data to all players)
		S: 'c_open_err|<error_id>' - connection unsuccesseful
		S: 'c_close|ok' - disconnection successeful
		<error_id>:
			'name_length' - name length > 32
			'name_exists' - name exists
			'conn_exists' - connection exists
		U: 'c_close' - close connection
		U: 'M<data>' - send message directly to the game engine
	USERS DATA:
		'c_users|<name1>,<state1>;<name2>,<state2>;...'
			<state?>: 'c'/'d' - connected/disconnected
*/

struct ac_conn {
	struct mg_str name;
	struct mg_connection* c;
	struct ac_conn* next;
};

struct ac_mgr {
	struct ac_conn* conns;
};

void ac_send_all(struct mg_connection* c, char* buf, int len) {
	struct ac_mgr* acmgr = (struct ac_mgr*)c->fn_data;
	struct ac_conn* acc = acmgr->conns;
	while (acc != NULL) {
		mg_ws_send(acc->c, buf, len, WEBSOCKET_OP_TEXT);
		acc = acc->next;
	}
}

#define WS_SEND_CONST(c_, arr_) mg_ws_send(c_, arr_, sizeof(arr_)-1, WEBSOCKET_OP_TEXT);

void ac_user_open(struct mg_connection* c, struct mg_str data) {
	// Connection check
	struct ac_mgr* acmgr = (struct ac_mgr*)c->fn_data;
	struct ac_conn* acc = acmgr->conns;
	while (acc != NULL) {
		if (c == acc->c) {
			WS_SEND_CONST(c, "c_open|conn_exists");
			return;
		}
		acc = acc->next;
	}
	// Check name
	if (data.len > 128 || u8_strlen(data.buf) > 32) {
		WS_SEND_CONST(c, "c_open_err|name_length");
		return;
	}
	if (u8_strstr(data.buf, ",") != NULL) {
		WS_SEND_CONST(c, "c_open_err|name_forbidden");
		return;
	}
	// Name/connection duplication check
	acc = acmgr->conns;
	while (acc != NULL) {
		if (mg_strcmp(data, acc->name) == 0) {
			WS_SEND_CONST(c, "c_open_err|name_exists");
			return;
		}
		acc = acc->next;
	}
	// Add to the list
	acc = calloc(1, sizeof(*acc));
	assert(acc);
	acc->name.buf = calloc(data.len, sizeof(char));
	assert(acc->name.buf);
	acc->name.len = data.len;
	strncpy(acc->name.buf, data.buf, data.len);
	acc->c = c;
	acc->next = NULL;
	if (acmgr->conns == NULL) {
		acmgr->conns = acc;
	} else {
		LIST_ADD_HEAD(struct ac_conn, &acmgr->conns, acc);
	}
	// TODO: free stuff
	sds usrstr = sdsnew("c_users|");
	while (acc != NULL) {
		usrstr = sdscat(usrstr, acc->name.buf);
		acc = acc->next;
		if (acc != NULL)
			usrstr = sdscat(usrstr, ",");
	}
	ac_send_all(c, usrstr, sdslen(usrstr));
	sdsfree(usrstr);
	WS_SEND_CONST(c, "c_open_ok");
}

void ac_user_close(struct mg_connection* c, struct mg_str data) {
	struct ac_mgr* acmgr = (struct ac_mgr*)c->fn_data;
	struct ac_conn** accp = &acmgr->conns;
	while (*accp != NULL) {
		if (c == (*accp)->c) {
			free((*accp)->name.buf);
			free((*accp));
			*accp = (*accp)->next;
			WS_SEND_CONST(c, "c_close|ok");
			break;
		}
		accp = &(*accp)->next;
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

typedef void (ac_handler)(struct mg_connection* c, struct mg_str data);

char ac_route(struct mg_connection* c, struct mg_str data, char* pre, size_t prelen, ac_handler* handler) {
	if (prelen > data.len || strncmp(pre, data.buf, prelen) != 0)
		return 0;
	struct mg_str handler_data = (struct mg_str){.buf=NULL,.len=0};
	if (prelen != data.len && prelen != data.len+1) {
		handler_data.buf = data.buf+prelen+1;
		handler_data.len = data.len-prelen-1;
	}
	handler(c, handler_data);
	return 1;
}

#define AC_ROUTE(pre_, handler_) \
	if (ac_route(c, wm->data, pre_, sizeof(pre_)-1, handler_)) return;

void ev_handle_ws_msg(struct mg_connection* c, void* ev_data) {
	struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
	//AC_ROUTE("M", ac_user_open)
	AC_ROUTE("c_open", ac_user_open)
	AC_ROUTE("c_close", ac_user_close)
}

void ev_handle_ws_close(struct mg_connection* c, void* ev_data) {
	//struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
	//disconnect
	// TODO: Add ac_user_close
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
	struct ac_mgr acmgr;
	mg_http_listen(&mgr, "http://0.0.0.0:6969", ev_handler, &acmgr);
	for (;;) {
		mg_mgr_poll(&mgr, 1000);
	}
	return 0;
}
