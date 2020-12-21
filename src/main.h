#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/udp.h>

#include "pthread.h"

#define PORT         50002
#define CONTENT_SIZE 64

int       sockfd;
pthread_t thsend;
pthread_t threcv;
int       count = 0;
