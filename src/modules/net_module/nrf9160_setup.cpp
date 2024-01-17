// The nRF9160 is one odd duck in the flock.
// Unlike its friends, it refuses to bring a driver dish to the kernel potluck, so we're DIY-ing it right here.

#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <modem/modem_key_mgmt.h>
#include <modem/nrf_modem_lib.h>
#include <modem/pdn.h>
#include <nrf_errno.h>
#include <nrf_modem.h>

#include "net_module.h"

#include <stdio.h>
#include <errno.h>

static const char server_cert[] = {
#include "../../../res/cert/ServerPublic.pem"
};

static const char hardware_private[] = {
#include "../../../res/cert/HardwarePrivateKey.pem"
};

static const char hardware_public[] = {
#include "../../../res/cert/HardwarePublicKey.pem"
};

// NRF_MODEM_LIB_ON_INIT(orecart_init_hook, on_modem_lib_init, NULL);

void pdn_event_handler(uint8_t cid, enum pdn_event event, int reason) {
	switch (event) {
	case PDN_EVENT_CNEC_ESM:
		printk("PDP context %d error, %s\n", cid, pdn_esm_strerror(reason));
		break;
	case PDN_EVENT_ACTIVATED:
		printk("PDP context %d activated\n", cid);
		break;
	case PDN_EVENT_DEACTIVATED:
		printk("PDP context %d deactivated\n", cid);
		break;
	case PDN_EVENT_NETWORK_DETACH:
		printk("PDP context %d network detached\n", cid);
		break;
	case PDN_EVENT_IPV6_UP:
		printk("PDP context %d IPv6 up\n", cid);
		break;
	case PDN_EVENT_IPV6_DOWN:
		printk("PDP context %d IPv6 down\n", cid);
		break;
	default:
		printk("PDP context %d, unknown event %d\n", cid, event);
		break;
	}
}

int cert_provision(void) {
	int err;
	bool exists;

	/* It may be sufficient for you application to check whether the correct
	 * certificate is provisioned with a given tag directly using modem_key_mgmt_cmp().
	 * Here, for the sake of the completeness, we check that a certificate exists
	 * before comparing it with what we expect it to be.
	 */
	err = modem_key_mgmt_exists(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);
	if (err) {
		printk("Failed to check for certificates err %d\n", err);
		return err;
	}

	if (exists) {
		int mismatch = modem_key_mgmt_cmp(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, server_cert,
					      strlen(server_cert));
		
		mismatch &= modem_key_mgmt_cmp(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT, hardware_private,
					      strlen(hardware_private));
		
		mismatch &= modem_key_mgmt_cmp(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT, hardware_public,
					      strlen(hardware_public));

		if (!mismatch) {
			printk("Certificate match!\n");
			return 0;
		}

		printk("Certificate mismatch\n");
		err = modem_key_mgmt_delete(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
		if (err) {
			printk("Failed to delete existing certificate, err %d\n", err);
		}
	}

	printk("Provisioning certificate\n");

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, server_cert, strlen(server_cert));
	if (err) {
		printk("Failed to provision certificate, err %d\n", err);
		return err;
	}

	/* Provision Private Certificate. */
	err = modem_key_mgmt_write(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT, hardware_private, strlen(hardware_private));
	if (err) {
		printk("NRF_CLOUD_CLIENT_PRIVATE_KEY err: %d", err);
		return err;
	}

	/* Provision Public Certificate. */
	err = modem_key_mgmt_write(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT, hardware_public, strlen(hardware_public));
	if (err) {
		printk("NRF_CLOUD_CLIENT_PUBLIC_CERTIFICATE err: %d", err);
		return err;
	}

	return 0;
}

void init_nrf9160_modem() {
	printk("Initializing the nRF9160 modem....\r\n");

	int err = nrf_modem_lib_init();
	if (err) {
		printk("Modem library initialization failed, error: %d\n", err);
		return;
	}

	err = cert_provision();
	if (err) {
		return;
	}

	printk("Waiting for network.. \r\n");

    err = lte_lc_init_and_connect();
	if (err) {
		printk("Failed to connect to the LTE network, err %d\n", err);
		return;
	}

	printk("nRF9160 successfully connected to the LTE network!\r\n");
}
