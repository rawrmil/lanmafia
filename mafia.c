#include "mongoose.h"
#include "jansson.h"

// E V E N T S

// EV: Room connections
/*
WebSockets:
- Client message - U
- Server message - S
- Message format: JSON ("type and other data fields)
Room connection:
- Connecting:
-- ws.send('{"type": "room/conn/open", "name": "Coolguy1337"}')
--- Check name<=32 bytes till '\0'
--- Check if not already connected
--- Renew the list and send to players
-- S: '{ "type": "room/conn/ok" }' - connection successeful
-- S: '{ "type": "room/conn/err", "error_id": "..." }'
---- "error_id":
----- "name-empty" - empty name
----- "name-long" - name too long
----- "name-invalid" - name contains invalid chars
*/

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

void ev_handle_ws_msg(struct mg_connection* c, void* ev_data) {
	// Testing: ws.send("something");
	struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
	printf("'wm->data.buf': '%s'\n", wm->data.buf);
	json_error_t* err;
	json_t* msg = json_loads(wm->data.buf, 0, err);
	if (!msg) {
		printf("ERR: WS message parsing\n");
		return;
	}
	json_t* type_json = json_object_get(msg, "type");
	if (!json_is_string(type_json)) {
		printf("ERR: No 'type' field in WS message\n");
		return;
	}
	printf("type: %s\n", json_string_value(type_json));
	json_decref(msg);
}

void ev_handle_ws_close(struct mg_connection* c, void* ev_data) {
	//struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
	//disconnect
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
