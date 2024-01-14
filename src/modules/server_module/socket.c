#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/net_ip.h>

#include "server_module.h"
#include "socket.h"

static const char cert[] = {
#include "server_dos.der.inc"
// #include "../../../res/cert/DigiCertGlobalRootCA.pem"
};

const char* hostName = SERVER_HOST;

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


static int setup_socket(struct sockaddr_in* family, const char *server, int *sock)
{
	printk("Setup socket!\r\n");

	*sock = socket(family, SOCK_STREAM, IPPROTO_TLS_1_2);

	if (*sock < 0) {
		printk("Failed to create HTTP socket (%d)", -errno);
	}
	
	printk("Attemping TLS....\r\n");

	int ret = setsockopt(*sock, SOL_TLS, TLS_CIPHERSUITE_LIST, &SUPPORTED_CIPHERS, sizeof(SUPPORTED_CIPHERS));
	if (ret < 0) {
		printk("Failed to set ciphersuite list");
		ret -errno;
	}

	ret = setsockopt(*sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list, sizeof(sec_tag_list));
	if (ret < 0) {
		printk("Failed to set secure option (%d)", -errno);
		ret = -errno;
	}

	printk("Hostname %s\r\n", hostName);

	ret = setsockopt(*sock, SOL_TLS, TLS_HOSTNAME, &hostName, sizeof(SERVER_HOST) - 1);
	if (ret < 0) {
		printk("Failed to set TLS_HOSTNAME option (%d)", -errno);
		ret = -errno;
	}

	int peer_verify = 0;
	ret = setsockopt(*sock, SOL_TLS, TLS_PEER_VERIFY, &peer_verify, sizeof(peer_verify));
	if (ret < 0) {
		printk("Failed to set to NOVERIFY!\r\n");
		ret = -errno;
	}

	int session_cache = TLS_SESSION_CACHE_ENABLED;

	ret = setsockopt(*sock, SOL_TLS, TLS_SESSION_CACHE, &session_cache,
					sizeof(session_cache));
	if (ret < 0)
	{
		printk("Failed to setup session cache, err %d", errno);
	}

	return ret;
}


int connect_socket(sa_family_t family, struct sockaddr* addr, const char *server, int *sock) {
	int ret;

	printk("Setup Socket!!\r\n");

	ret = setup_socket(family, server, sock);
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