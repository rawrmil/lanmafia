#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "mongoose.h"
#include "sds.h"

pthread_mutex_t signal_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t signal_cond = PTHREAD_COND_INITIALIZER;

char signal_ready = 0;
sds signal_message = NULL;

void* ws_thread_func(void* arg) {
	int a = 0;
	for (int i = 0; i < 10; i++)
		a += 1;
	pthread_mutex_lock(&signal_mutex);
	signal_ready = 1;
	char buf[32];
	snprintf(buf, sizeof(buf), "BLAH: %d", a);
	signal_message = sdsnewlen(buf, sizeof(buf));
	pthread_cond_signal(&signal_cond);
	pthread_mutex_unlock(&signal_mutex);
}

int main() {
	printf("BACKEND TESTS:\n");
	pthread_t thread;
	pthread_create(&thread, NULL, ws_thread_func, NULL);

	pthread_mutex_lock(&signal_mutex);
	while (!signal_ready) {
		pthread_cond_wait(&signal_cond, &signal_mutex);
	}
	pthread_mutex_unlock(&signal_mutex);

	pthread_join(thread, NULL);

	printf("MESSAGE: '%s'\n", signal_message);
	return 0;
}
