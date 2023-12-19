#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>

#include <mbedtls/platform.h>
#include <mbedtls/ssl_ciphersuites.h>

#include "server_module.h"

static const char cert[] = {
#include "server_dos.der.inc"
// #include "../../../res/cert/DigiCertGlobalRootCA.pem"
};

char hostName[] = SERVER_HOST;

#define TLS_SEC_TAG 1

sec_tag_t sec_tag_list[] = {
	TLS_SEC_TAG,
};


static int setup_socket(sa_family_t family, const char *server, int port,
			int *sock, struct sockaddr *addr, socklen_t addr_len)
{
	const char *family_str = family == AF_INET ? "IPv4" : "IPv6";
	int ret = 0;

	memset(addr, 0, addr_len);

	if (family == AF_INET) {
		net_sin(addr)->sin_family = AF_INET;
		net_sin(addr)->sin_port = htons(port);
		inet_pton(family, server, &net_sin(addr)->sin_addr);
	} else {
		net_sin6(addr)->sin6_family = AF_INET6;
		net_sin6(addr)->sin6_port = htons(port);
		inet_pton(family, server, &net_sin6(addr)->sin6_addr);
	}

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS)) {
	
		int ciphersuite_list[] = {
			MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA,
			MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256,
			MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256
		};

		*sock = socket(family, SOCK_STREAM, IPPROTO_TLS_1_2);

		printk("BOB: %d %d\r\n", strlen(cert), sizeof(cert));
		
		ret = tls_credential_add(sec_tag_list[0], TLS_CREDENTIAL_CA_CERTIFICATE, cert, sizeof(cert));

		if (ret < 0) {
			printk("Failed to registed certificate!\r\n");
		} else {
			printk("Successfully registed certificate\r\n");
		}

		struct timeval timeout = {
			.tv_sec = 5
		};

		if (*sock >= 0) {
			printk("Attemping TLS....\r\n");

			// ret = setsockopt(*sock, SOL_TLS, TLS_CIPHERSUITE_LIST, &ciphersuite_list, sizeof(ciphersuite_list));

			ret = setsockopt(*sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list, sizeof(sec_tag_list));
			if (ret < 0) {
				printk("Failed to set %s secure option (%d)", family_str, -errno);
				ret = -errno;
			}

			ret = setsockopt(*sock, SOL_TLS, TLS_HOSTNAME, &hostName, sizeof(SERVER_HOST));
			if (ret < 0) {
				printk("Failed to set %s TLS_HOSTNAME option (%d)", family_str, -errno);
				ret = -errno;
			}

			int peer_verify = 0;
			ret = setsockopt(*sock, SOL_TLS, TLS_PEER_VERIFY, &peer_verify, sizeof(peer_verify));
			if (ret < 0) {
				printk("Failed to set to NOVERIFY!\r\n");
				ret = -errno;
			}

			setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
			setsockopt(*sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
		}
	} else {
		*sock = socket(family, SOCK_STREAM, IPPROTO_TCP);
	}

	if (*sock < 0) {
		printk("Failed to create %s HTTP socket (%d)", family_str,
			-errno);
	}

	return ret;
}


int connect_socket(sa_family_t family, const char *server, int port,
			  int *sock, struct sockaddr *addr, socklen_t addr_len) {
	int ret;

	ret = setup_socket(family, server, port, sock, addr, addr_len);
	if (ret < 0 || *sock < 0) {
		return -1;
	}

	ret = connect(*sock, addr, addr_len);
	if (ret < 0) {
		printk("Cannot connect to %s remote (%d)",
			family == AF_INET ? "IPv4" : "IPv6",
			-errno);
		ret = -errno;
	}

	return ret;
}