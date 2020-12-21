/**
 * @file vpn.c
 * @author BadCodeBuilder
 * @brief 
 * @version 0.1
 * @date 2020-12-21
 * 
 */

#include "vpn.h"

/**
 * @brief make print_log head
 * 
 * @param level 
 * @param buff 
 * @return int 
 */
int plog_head(int level, char* buff) {
    char str_time[20] = {0};
    char* str_log_level = NULL;
    time_t t = time(NULL);
    struct tm* now = localtime(&t);

    strftime(str_time, 20, "%Y-%m-%dT%H:%M:%S", now);
    switch (level) {
    case DEBUG:
        str_log_level = "DEBUG";
        break;
    case INFO:
        str_log_level = "INFO";
        break;
    case WARNING:
        str_log_level = "WARN";
        break;
    case ERROR:
    case ERROR_CUSTOM:
        str_log_level = "ERR";
        break;
    default:
        break;
    }
    snprintf(buff, 35, "[%s] [%s]", str_time, str_log_level);
    return 0;
}

/**
 * @brief print log
 * 
 * @param level 
 * @param str 
 * @return int 
 * @todo it will be used to improve log system
 */
int plog(int level, const char* str) {
#ifndef DEBUG_MOD
    if (level == DEBUG) {
        return 0;
    }
#endif
    char buff[40] = {0};
    plog_head(level, buff);
    if (level == ERROR) {
        perror(buff);
    } else {
        printf("%s %s\n", buff, str);
    }
    return 0;
}

/**
 * @brief create a tun device
 * 
 * @return int 
 */
int tun_alloc() {
    int res;

    tunfd = open("/dev/net/tun", O_RDWR);
    if (tunfd < 0) {
        // perror("Open TUN Error");
        plog(ERROR, NULL);
        return 1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN|IFF_NO_PI;

    res = ioctl(tunfd, TUNSETIFF, &ifr);
    if (res < 0) {
        close(tunfd);
        perror("Config TUN Error");
        return res;
    }

    strncpy(tun_name, ifr.ifr_name, IF_NAMESIZE);

    printf("Open TUN decide: %s for reading and writing ...\n", tun_name);
    return 0;
}

/**
 * @brief turn up the tun device
 * 
 * @return int 
 */
int tun_up() {
    char cmd_buff[BUFSIZ] = {0};
    snprintf(cmd_buff, BUFSIZ, "sudo ip addr add %s dev %s", LOCAL_IP, tun_name);
    system(cmd_buff);
    snprintf(cmd_buff, BUFSIZ, "sudo ip link set %s up", tun_name);
    system(cmd_buff);

    system("sudo -k");
    return 0;
}

/**
 * @brief turn down the tun device
 * @deprecated not use
 * 
 * @return int execute result, default 0
 */
int tun_down() {
    char cmd_buff[BUFSIZ] = {0};
    snprintf(cmd_buff, BUFSIZ, "sudo ip link set %s down", tun_name);
    system(cmd_buff);
    snprintf(cmd_buff, BUFSIZ, "sudo ip addr del %s dev %s", LOCAL_IP, tun_name);
    system(cmd_buff);

    system("sudo -k");
    return 0;
}

/**
 * @brief send thread function
 * 
 * @param args 
 * @return void* 
 */
void* sender(void* args) {
    struct sockaddr_in to_addr           = *((struct sockaddr_in*)args);
    socklen_t          len               = sizeof(to_addr);
    char               read_buff[BUFSIZ] = {0};
    uint8_t            send_buff[BUFSIZ] = {0};
    ssize_t            n_read;
    ssize_t            n_send;
    while (1) {
        memset(read_buff, 0, BUFSIZ);
        n_read = read(tunfd, read_buff, BUFSIZ);
        if (n_read < 0) {
            perror("[TUN] read Error");
        } else if (n_read > 0) {
            printf("[TUN] read sth, %ld bytes\n", n_read);
            memset(encrypt_mask, 0, MASK_SIZE);
            AES_cbc_encrypt((const uint8_t*)read_buff, send_buff, (size_t)n_read, &session_encrypt_key, encrypt_mask, AES_ENCRYPT);
            // !!! VERY IMPORTANT CODE !!!
            // Function AES_cbc_encrypt encrypts and outputs 16 bytes 
            // each time. So you may know why I have to send more than
            // what I read from tun. Otherwise, it is a broken block,
            // which will be decrypted incorrectly.
            int send_count;
            if (n_read % 16 == 0) {
                send_count = n_read;
            } else {
                send_count = ((n_read >> 4)+1)<<4;
            }

            n_send = sendto(sockfd, (char*)send_buff, send_count, 0, (const struct sockaddr*)&to_addr, len);
            if (n_send < 0) {
                perror("[error] send Error");
            }
            printf("[SOCK] send %ld bytes\n", n_send);
        }
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
    struct sockaddr from_addr;
    socklen_t       len;
    char            recv_buff[BUFSIZ]  = {0};
    uint8_t         write_buff[BUFSIZ] = {0};
    ssize_t         n_recv;
    ssize_t         n_write;
    while (1) {
        memset(recv_buff, 0, BUFSIZ);
        memset(write_buff, 0, BUFSIZ);
        n_recv = recvfrom(sockfd, recv_buff, BUFSIZ, 0, &from_addr, &len);
        if (n_recv < 0) {
            perror("[error] recv Error");
        } else if (n_recv > 0) {
            printf("[recv] recv %ld bytes\n", n_recv);
            memset(decrypt_mask, 0, MASK_SIZE);
            AES_cbc_encrypt((const uint8_t*)recv_buff, write_buff, (size_t)n_recv, &session_decrypt_key, decrypt_mask, AES_DECRYPT);
            n_write = write(tunfd, (char*)write_buff, n_recv);
            if (n_write < 0) {
                perror("[error] write Error");
            }
            printf("[TUN] write %ld bytes\n", n_write);
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
        pthread_cancel(thsend);
        pthread_cancel(threcv);
        close(sockfd);
        // tun_down();   // It turns down autumatically
        close(tunfd);
        printf("[finish] done!\n");
        exit(0);
    }
    return;
}

/**
 * @brief initialize encrypt and decrypt key(they shoule be exactly alike)
 * 
 * @return int 
 */
int init_key() {
    AES_set_encrypt_key(str_key, 128, &session_encrypt_key);
    AES_set_decrypt_key(str_key, 128, &session_decrypt_key);
    return 0;
}

int main(int argc, char* argv[]) {
    int res;

    res = tun_alloc();
    if (res != 0) {
        return res;
    }
    tun_up();

    init_key();

    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket Error");
        return 1;
    }

    signal(SIGINT, sig_handle);

    struct sockaddr_in srv_addr;
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(VPN_PORT);
    socklen_t len = sizeof(srv_addr);

    res = bind(sockfd, (const struct sockaddr*)&srv_addr, len);
    if (res < 0) {
        perror("Bind Error");
        close(sockfd);
        return 1;
    }

    struct sockaddr_in to_addr;
    to_addr.sin_family = AF_INET;
    to_addr.sin_addr.s_addr = inet_addr(PEER_IP);
    to_addr.sin_port = htons(VPN_PORT);

    res = pthread_create(&threcv, NULL, recver, NULL);
    if (res != 0) {
        printf("Pthread Error\n");
        return 1;
    }
    printf("Receiving ...\n");
    res = pthread_create(&thsend, NULL, sender, (void*)&to_addr);
    if (res != 0) {
        printf("Pthread Error\n");
        return 1;
    }
    printf("Sending ...\n");

    while (1);
    return 0;
}