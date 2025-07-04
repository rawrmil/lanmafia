#include "mongoose.h"

// E V E N T S

// EV: Room connections
/*
 * Room connection:
 * Connecting:
 *   REQ: 'rc<name>' - connect to the room with name
 *   - Check name<=32 bytes till '\0'
 *   - Check if not already connected
 *   RES: 'rs' - connection successeful
 *   RES: 're<error_id>' - connection unsuccesseful
 *   Error ID:
 *   - 'e' - empty command
 *   - 'E' - empty name
 *   - 'l' - name too long
 * On new connection:
 *   ALL: 'rl<name>\x1f<name>...'
 */

void ev_user_room_conn(struct mg_connection* c, char* msg) {
	mg_ws_send(c, msg, strlen(msg), WEBSOCKET_OP_TEXT);
}

// EV: High level handlers

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

void ev_handle_mg_msg(struct mg_connection* c, void* ev_data) {
	// Testing: ws.send("something");
	struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
	if (wm->data.len < 2) {
		// TODO: Empty msg error
		return;
	}
	switch (wm->data.buf[0]) {
		case 'r':
			ev_user_room_conn(c, &wm->data.buf[1]);
			break;
	}
	printf("%s\n", wm->data.buf);
}

void ev_handle_mg_close(struct mg_connection* c, void* ev_data) {
	//struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
	//disconnect
}

void ev_handler(struct mg_connection* c, int ev, void* ev_data) {
	switch (ev) {
		case MG_EV_HTTP_MSG:
			ev_handle_http_msg(c, ev_data);
			break;
		case MG_EV_WS_MSG:
			ev_handle_mg_msg(c, ev_data);
			break;
		case MG_EV_CLOSE:
			ev_handle_mg_close(c, ev_data);
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
