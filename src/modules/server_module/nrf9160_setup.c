// The nRF9160 is one odd duck in the flock.
// Unlike its friends, it refuses to bring a driver dish to the kernel potluck, so we're DIY-ing it right here.

#include <nrf_modem.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <modem/nrf_modem_lib.h>
#include <modem/pdn.h>
#include <modem/modem_key_mgmt.h>

#include "server_module.h"

static const char cert[] = {
#include "../../../res/cert/DigiCertGlobalRootCA.pem"
};

NRF_MODEM_LIB_ON_INIT(orecart_init_hook, on_modem_lib_init, NULL);

#define TLS_SEC_TAG 42

void pdn_event_handler(uint8_t cid, enum pdn_event event, int reason)
{
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

int cert_provision(void)
{
	int err;
	bool exists;
	int mismatch;

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
		mismatch = modem_key_mgmt_cmp(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, cert,
					      strlen(cert));
		if (!mismatch) {
			printk("Certificate match\n");
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
	err = modem_key_mgmt_write(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, cert,
				   sizeof(cert) - 1);
	if (err) {
		printk("Failed to provision certificate, err %d\n", err);
		return err;
	}

	return 0;
}

static void on_modem_lib_init(int ret, void *ctx)
{
	int err;

	if (ret != 0) return;

    err = pdn_default_ctx_cb_reg(pdn_event_handler);
	if (err) {
		LOG_ERR("pdn_default_ctx_cb_reg, error: %d", err);
		return;
	}

	err = cert_provision();
	if (err) {
		return 0;
	}

    err = lte_lc_init_and_connect();
	if (err) {
		printk("Failed to connect to the LTE network, err %d\n", err);
		return;
	}
	

	k_sem_give(&modem_available);
}