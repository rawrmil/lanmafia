#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "mongoose.h"
#include "sds.h"

#ifdef VERBOSE
#define LOG_VERBOSE(fmt, ...) if (VERBOSE) printf(fmt, ##__VA_ARGS__)
#else
#define LOG_VERBOSE(fmt, ...)
#endif

// C O N N E C T I O N

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
				LOG_VERBOSE("WS: OPEN: ");
				struct ut_ws_conn* utwsc = (struct ut_ws_conn*)c->fn_data;
				LOG_VERBOSE("INDEX: %d\n", utwsc->index);
				utwsc->is_connected = 1;
			}
			break;
		case MG_EV_WS_MSG:
			{
				LOG_VERBOSE("WS: MSG: ");
				struct ut_ws_conn* utwsc = (struct ut_ws_conn*)c->fn_data;
				LOG_VERBOSE("INDEX: %d\n", utwsc->index);
				struct mg_ws_message *wm = (struct mg_ws_message*)ev_data;
				LOG_VERBOSE("DATA: '%.*s'\n", wm->data.len, wm->data.buf);
				utwsc->responce = sdsnewlen(wm->data.buf, wm->data.len);
				utwsc->got_responce = 1;
			}
			break;
		case MG_EV_CLOSE:
			break;
		case MG_EV_ERROR:
			{
				LOG_VERBOSE("WS: ERROR: ");
				struct ut_ws_conn* utwsc = (struct ut_ws_conn*)c->fn_data;
				printf("FATAL: Couldn't establish %d connection\n", utwsc->index);
				exit(1);
			}
			break;
	}
}

// U T I L S

char ut_success;

char ut_connect(struct mg_connection* c) {
	struct ut_ws_conn* utwsc = (struct ut_ws_conn*)c->fn_data;
	utwsc->is_connected = 0;
	while (!utwsc->is_connected) {
		mg_mgr_poll(c->mgr, 1000);
	}
	LOG_VERBOSE("Connection %d established\n", utwsc->index);
}

void ut_send(struct mg_connection* c, char* msg) {
	LOG_VERBOSE("SEND: %s\n");
	mg_ws_send(c, msg, strlen(msg), WEBSOCKET_OP_TEXT);
}

void ut_expect(struct mg_connection* c, char* res) {
	struct ut_ws_conn* utwsc = (struct ut_ws_conn*)c->fn_data;
	utwsc->got_responce = 0;
	while (!utwsc->got_responce) {
		mg_mgr_poll(c->mgr, 1000);
	}
	LOG_VERBOSE("EXPECTED: '%s'\n", res);
	LOG_VERBOSE("RESPONCE: '%s'\n", utwsc->responce);
	if (strcmp(utwsc->responce, res) != 0)
		return;
	sdsfree(utwsc->responce);
	ut_success |= 1;
}

// T E S T S

char test_conn_open(struct mg_connection* c[]) {
	printf("!!!\n");
	ut_send(c[0], "c_open|Coolname666");
	ut_expect(c[0], "c_open_ok");
	ut_expect(c[0], "c_users|Coolname666");
	return 1;
}

#define UNIT_TEST(server_thread_, c_, name_, testfunc_)                  \
	do {                                                                   \
		ut_success = 0;                                                      \
		pthread_join(server_thread, NULL);                                   \
		printf("UNIT-TEST: %s: %s", name_, testfunc_(c_) ? "✔️OK" : "❌ERR"); \
		pthread_cancel(server_thread);                                       \
	} while (0);

// M A I N

void a_main(int argc, char* argv[]); // From mafia.c

void* server_thread_func(void* arg) {
	char* argv[] = { "mafia", "--port", "7998" };
	a_main(3, argv);
}

int main() {
	printf("BACKEND UNIT-TESTS:\n");

	LOG_VERBOSE("Initializing server thread...\n");

	pthread_t server_thread;
	if (pthread_create(&server_thread, NULL, server_thread_func, NULL)) {
		printf("FATAL: Cannot create a thread");
		return 1;
	}

	sleep(1);

	LOG_VERBOSE("Initializing WS connections...\n");

	struct mg_mgr mgr;
	struct mg_connection* c[16];
	struct ut_ws_conn utwsc[16] = {0};
	mg_mgr_init(&mgr);
	for (int i = 0; i < 16; i++) {
		utwsc[i].index = i;
		LOG_VERBOSE("Connection %d... \n", i);
		c[i] = mg_ws_connect(&mgr, "http://localhost:7998/ws", ev_handle, &utwsc[i], NULL);
	}

	LOG_VERBOSE("Begin testing...\n");

	ut_connect(c[0]);

	UNIT_TEST(server_thread, c, "CONNECTION OPEN", test_conn_open);

	return 0;
}
