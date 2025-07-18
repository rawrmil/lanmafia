#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mongoose.h"
#include "sds.h"

struct ut_ws_conn {
	sds responce;
};

void ev_handle(struct mg_connection* c, int ev, void* ev_data) {
	switch (ev) {
		case MG_EV_WS_OPEN:
			printf("CONNECTION SUCCESSEFUL\n");
			break;
		case MG_EV_WS_MSG:
			//struct mg_ws_message *wm = (struct mg_ws_message*)ev_data;
			break;
		case MG_EV_CLOSE:
			break;
	}
}

//enum ut_action_type {
//	AT_SEND = 0,
//	AT_EXPECT,
//}
//
//struct ut_action {
//	ut_action_type type;
//	char* buf;
//}

int main() {
	printf("BACKEND TESTS:\n");
	struct mg_mgr mgr[16];
	struct mg_connection* wsc[16];
	struct ut_ws_conn utwsc[16];
	for (int i = 0; i < 16; i++) {
		mg_mgr_init(&mgr[i]);
		wsc[i] = mg_ws_connect(&mgr[i], "http://localhost:6969/ws", ev_handle, &utwsc[i], NULL);
		if (wsc[i] == NULL) {
			printf("WS-SERVER CONNECTION ERROR\n");
			return 1;
		}
	}
	for (;;) {
		for (int i = 0; i < 16; i++) {
			mg_mgr_poll(&mgr[i], 1000);
		}
	}
	return 0;
}
