#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/net_ip.h>

#include "socket.h"

#define TLS_SEC_TAG 42

sec_tag_t sec_tag_list[] = {
	TLS_SEC_TAG,
};

const uint32_t SUPPORTED_CIPHERS[] = {
	TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
	TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,
	TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
	TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384,
	TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA,
	TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256,
	TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA,
	TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA,
	TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256,
	TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA,
	TLS_PSK_WITH_AES_256_CBC_SHA,
	TLS_PSK_WITH_AES_128_CBC_SHA256,
	TLS_PSK_WITH_AES_128_CBC_SHA,
	TLS_PSK_WITH_AES_128_CCM_8
};


static int setup_socket(int family, const char *hostname, int *sock) {
	printk("[DEBUG] Setup socket!\n");

	*sock = socket(family, SOCK_STREAM, IPPROTO_TLS_1_2);

	if (*sock < 0) {
		printk("Failed to create HTTP socket (%d)", -errno);
	}
	
	printk("Attemping TLS....\r\n");

	int ret = setsockopt(*sock, SOL_TLS, TLS_CIPHERSUITE_LIST, &SUPPORTED_CIPHERS, sizeof(SUPPORTED_CIPHERS));
	if (ret < 0) {
		printk("Failed to set ciphersuite list");
	}

	ret = setsockopt(*sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list, sizeof(sec_tag_list));
	if (ret < 0) {
		printk("Failed to set secure option (%d)", -errno);
	}

	printk("Hostname %s\r\n", hostname);

	ret = setsockopt(*sock, SOL_TLS, TLS_HOSTNAME, &hostname, strlen(hostname) - 1);
	if (ret < 0) {
		printk("Failed to set TLS_HOSTNAME option (%d)", -errno);
	}

	int peer_verify = 0;
	ret = setsockopt(*sock, SOL_TLS, TLS_PEER_VERIFY, &peer_verify, sizeof(peer_verify));
	if (ret < 0) {
		printk("Failed to set to NOVERIFY!\r\n");
	}

	int session_cache = TLS_SESSION_CACHE_ENABLED;

	ret = setsockopt(*sock, SOL_TLS, TLS_SESSION_CACHE, &session_cache,
					sizeof(session_cache));
	if (ret < 0) {
		printk("Failed to setup session cache, err %d", errno);
	}

	return ret;
}


int connect_socket(int family, struct sockaddr* addr, const char* hostname, int* sock) {
	int ret;

	ret = setup_socket(family, hostname, sock);
	if (ret < 0 || *sock < 0) {
		return -1;
	}

	printk("Connect Socket!!\r\n");

	ret = connect(*sock, addr, sizeof(struct sockaddr_in));
	if (ret < 0) {
		printk("Cannot connect to %s remote (%d)",
			family == AF_INET ? "IPv4" : "IPv6",
			-errno);
		ret = -errno;
	}

	return ret;
}