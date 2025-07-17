#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthreads.h>

#include "mongoose.c"

void* client_thread(void *arg) {
	
}

int main() {
	struct mg_mgr mgr;
	mg_mgr_init(&mgr);
	return 0;
}
