#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mongoose.h"
#include "sds.h"
#include "klib/klist.h"

#define LOG_DEBUG(format_, ...) \
	if (DEBUG) printf(format_, ##__VA_ARGS__);

KLIST_INIT(respque, sds, free)

struct ws_conn_data {
	klist_t(respque)* respque;
	unsigned is_connected : 1;
};

void ws_ev_handler(struct mg_connection* c, int ev, void* ev_data) {
	struct ws_conn_data* cd = (struct ws_conn_data*)c->fn_data;
	switch (ev) {
		case MG_EV_WS_OPEN:
			LOG_DEBUG("WS_OPEN\n");
			cd->respque = kl_init(respque);
			cd->is_connected = 1;
			break;
		case MG_EV_WS_MSG:
			LOG_DEBUG("WS_MSG\n");
			struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
			sds resp = sdsnewlen(wm->data.buf, wm->data.len);
			*kl_pushp(respque, cd->respque) = resp;
			break;
		case MG_EV_CLOSE:
			LOG_DEBUG("WS_CLOSE\n");
			free(cd);
			break;
		case MG_EV_ERROR:
			LOG_DEBUG("WS_ERROR\n");
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
	LOG_DEBUG("SEND:   '%s'\n", msg);
	mg_ws_send(c, msg, strlen(msg), WEBSOCKET_OP_TEXT);
	mg_mgr_poll(c->mgr, 1000);
}

char ws_expect(struct mg_connection* c, char* exp) {
	struct ws_conn_data* cd = (struct ws_conn_data*)c->fn_data;
	sds resp;
	for (int i = 0;; i++) {
		LOG_DEBUG("POLL: %d\n", i);
		if (kl_shift(respque, cd->respque, &resp) == 0)
			break;
		if (i == 3) {
			// TODO: Error
			exit(1);
		}
		mg_mgr_poll(c->mgr, 1000);
	}
	LOG_DEBUG("EXPECT: '%s'\n", exp);
	LOG_DEBUG("GOT:    '%s'\n", resp);
	return strcmp(exp, resp) == 0;
	sdsfree(resp);
}

void ws_restart(struct mg_mgr* mgr) {
	mg_ws_send(mgr->conns, "c_server_restart", 16, WEBSOCKET_OP_TEXT);
	mg_mgr_free(mgr);
	mg_mgr_poll(mgr, 1000);
}

// T E S T S

#define UT_ASSERT(cond_) \
	if (!(cond_)) { \
		printf("  UT_ASSERT: %s\n", #cond_); \
		return 0; \
		if (DEBUG) exit(1); \
	}

//#define UT_STRCMP(str1_, str2_) \
//	if (strcmp(str1_, str2_) != 0) { \
//		printf("  UT_STRCMP: '%s' != '%s'"); \
//		return 0; \
//	}

#define UT_DEFINE_CONN_DATA() \
	struct mg_mgr mgr; \
	struct mg_connection* c[16] = {0}; \
	mg_mgr_init(&mgr);

char utf_conn_part1() {
	UT_DEFINE_CONN_DATA();
	ws_connect(&mgr, &c[0]);
	ws_send(c[0], "c_open|Someguy");
	UT_ASSERT(ws_expect(c[0], "c_open_ok"));
	UT_ASSERT(ws_expect(c[0], "c_users|Someguy"));
	ws_restart(&mgr);
	return 1;
}

char utf_conn_part2() {
	UT_DEFINE_CONN_DATA();
	ws_connect(&mgr, &c[0]);
	ws_connect(&mgr, &c[1]);
	ws_connect(&mgr, &c[2]);
	ws_send(c[0], "c_open|Coolguy");
	UT_ASSERT(ws_expect(c[0], "c_open_ok"));
	UT_ASSERT(ws_expect(c[0], "c_users|Coolguy"));
	ws_send(c[1], "c_open|Lame!guy");
	UT_ASSERT(ws_expect(c[1], "c_open_ok"));
	UT_ASSERT(ws_expect(c[1], "c_users|Lame!guy,Coolguy"));
	ws_send(c[2], "c_open|Он русский");
	UT_ASSERT(ws_expect(c[2], "c_open_ok"));
	UT_ASSERT(ws_expect(c[2], "c_users|Он русский,Lame!guy,Coolguy"));
	ws_restart(&mgr);
	return 1;
}

// M A I N

#define UNIT_TEST(utf_) \
		printf("%d. %s: %s\n", ut_count++, #utf_, utf_() ? "[SUCC]" : "[FAIL]");

int main() {
	size_t ut_count = 0;
	UNIT_TEST(utf_conn_part1);
	UNIT_TEST(utf_conn_part2);
}
