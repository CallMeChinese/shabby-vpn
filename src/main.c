#include "main.h"

/**
 * @brief construct a packet, with fixed format
 * 
 * @param buff 
 * @return int 
 */
int make_buff(char* buff) {
    time_t t = time(NULL);
    strncpy(buff, ctime(&t), 30);
    buff[strlen(buff)-1] = ':';

    char content[CONTENT_SIZE] = {0};
    snprintf(content, CONTENT_SIZE, " hello, world, message %d", count);
    ++count;

    strncat(buff, content, CONTENT_SIZE);
    return 0;
}

/**
 * @brief send thhread function
 * 
 * @param args 
 * @return void* 
 */
void* sender(void* args) {
    struct sockaddr* to_addr      = (struct sockaddr*)args;
    char             buff[BUFSIZ] = {0};
    ssize_t          len;
    while (1) {
        make_buff(buff);
        len = sendto(sockfd, buff, strlen(buff), 0, to_addr, sizeof(*to_addr));
        if (len < 0) {
            perror("[send Error] ");
        } else {
            printf("[send] %s\n", buff);
        }

        sleep(2);
    }
    return NULL;
}

/**
 * @brief receive thread function
 * 
 * @param args 
 * @return void* 
 */
void* recver(void* args) {
    char            buff[BUFSIZ] = {0};
    struct sockaddr from_addr;
    socklen_t       len;
    ssize_t         packLen;
    while (1) {
        packLen = recvfrom(sockfd, buff, BUFSIZ, 0, &from_addr, &len);
        if (packLen < 0) {
            perror("[recv Error] ");
        } else {
            printf("[recv] %s\n", buff);
        }
    }
    return NULL;
}

/**
 * @brief signal handle function
 * 
 * @param signum 
 */
void sig_handle(int signum) {
    if (signum == SIGINT) {
        printf("\n[finish] finishing ...\n");
#ifdef SEND
        pthread_cancel(thsend);
#endif
#ifdef RECV
        pthread_cancel(threcv);
#endif
        close(sockfd);
        printf("[finish] done!\n");
        exit(0);
    }
    return;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Argument Error: too many or few args\n");
        return 1;
    }

    signal(SIGINT, sig_handle);

    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket Error");
        return 1;
    }

    struct sockaddr_in srv_addr;
    socklen_t len;
    srv_addr.sin_family      = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port        = htons(PORT);
    len = sizeof(srv_addr);

    int res;
    res = bind(sockfd, (const struct sockaddr*)&srv_addr, len);
    if (res < 0) {
        perror("Bind Error");
        return 1;
    }

    struct sockaddr_in to_addr;
    to_addr.sin_family      = AF_INET;
    to_addr.sin_addr.s_addr = inet_addr(argv[1]);
    to_addr.sin_port        = htons(PORT);

#ifdef RECV
    res = pthread_create(&threcv, NULL, recver, NULL);
    if (res != 0) {
        printf("Pthread Error\n");
        return 1;
    }
    printf("Receiving ...\n");
#endif

#ifdef SEND
    res = pthread_create(&thsend, NULL, sender, (void*)&to_addr);
    if (res != 0) {
        printf("Pthread Error\n");
        return 1;
    }
    printf("Sending ...\n");
#endif

    while (1);
    return 0;
}