/**
 * @file vpn.h
 * @author BadCodeBuilder
 * @brief 
 * @version 0.1
 * @date 2020-12-21
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/udp.h>

#include <openssl/crypto.h>
#include <openssl/aes.h>
#include "pthread.h"

/**
 * @brief IP of tun device. When application(not this vpn) send data
 * make sure data will go out from this device. \n
 * In most condition, you will let kernal select which device should be
 * used. So the easiest way is setting the IP of tun in the same net 
 * with the dst of the packet.
 * 
 */
#define LOCAL_IP        "10.11.12.100/24"
/**
 * @brief IP of another host. It should be a public IP address to you.
 * you have to communicate with it by this IP.
 * 
 */
#define PEER_IP         "192.168.137.99"
#define VPN_PORT        50003

#define DEBUG           0
#define INFO            1
#define WARNING         2
#define ERROR           3
#define ERROR_CUSTOM    4

#define MASK_SIZE       16

int           tunfd;
int           sockfd;
char          tun_name[IF_NAMESIZE];
pthread_t     thsend;
pthread_t     threcv;

// Cryption related variates
AES_KEY       session_encrypt_key;
AES_KEY       session_decrypt_key;
const uint8_t str_key[16] = "hello,world 2020";
uint8_t       encrypt_mask[MASK_SIZE];
uint8_t       decrypt_mask[MASK_SIZE];
