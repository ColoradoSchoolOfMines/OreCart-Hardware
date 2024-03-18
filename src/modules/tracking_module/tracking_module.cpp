#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <nrf_modem_at.h>
#include <nrf_modem_gnss.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>

#include "tracking_exception.h"
#include "../../common/location.h"

static nrf_modem_gnss_pvt_data_frame last_pvt;
K_MSGQ_DEFINE(nmea_queue, sizeof(struct nrf_modem_gnss_nmea_data_frame *), 10, 4);
static K_SEM_DEFINE(pvt_data_sem, 0, 1);
static K_SEM_DEFINE(time_sem, 0, 1);

TrackingException::TrackingException(std::string & exception) : _exception(exception) {}
TrackingException::TrackingException(const char * exception) : _exception(exception) {}

static void _gnss_event_handler(int event);

void tracking_module_init() {
  if (nrf_modem_gnss_event_handler_set(_gnss_event_handler) != 0) {
    throw TrackingException("Failed to set GNSS event handler");
  }

  // Enable all NMEA messages.
  uint16_t nmea_mask = NRF_MODEM_GNSS_NMEA_RMC_MASK |
    NRF_MODEM_GNSS_NMEA_GGA_MASK |
    NRF_MODEM_GNSS_NMEA_GLL_MASK |
    NRF_MODEM_GNSS_NMEA_GSA_MASK |
    NRF_MODEM_GNSS_NMEA_GSV_MASK;

  if (nrf_modem_gnss_nmea_mask_set(nmea_mask) != 0) {
    throw TrackingException("Failed to set GNSS mask");
  }

  /* Make QZSS satellites visible in the NMEA output. */
  if (nrf_modem_gnss_qzss_nmea_mode_set(NRF_MODEM_GNSS_QZSS_NMEA_MODE_CUSTOM) != 0) {
    LOG_WRN("Failed to enable custom QZSS NMEA mode");
  }

  /* This use case flag should always be set. */
  uint8_t use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START;

  if (IS_ENABLED(CONFIG_GNSS_SAMPLE_LOW_ACCURACY)) {
    use_case |= NRF_MODEM_GNSS_USE_CASE_LOW_ACCURACY;
  }

  if (nrf_modem_gnss_use_case_set(use_case) != 0) {
    LOG_WRN("Failed to set GNSS use case");
  }

  /* Default to no power saving. */
  uint8_t power_mode = NRF_MODEM_GNSS_PSM_DISABLED;

  if (nrf_modem_gnss_power_mode_set(power_mode) != 0) {
    throw TrackingException("Failed to set GNSS power saving mode");
  }

  // Continuous Tracking
  uint16_t fix_retry = 0;
  uint16_t fix_interval = 1;

  if (nrf_modem_gnss_fix_retry_set(fix_retry) != 0) {
    throw TrackingException("Failed to set GNSS fix retry");
  }

  if (nrf_modem_gnss_fix_interval_set(fix_interval) != 0) {
    throw TrackingException("Failed to set GNSS fix interval");
  }

  if (nrf_modem_gnss_start() != 0) {
    throw TrackingException("Failed to start GNSS");
  }	
}

static void _gnss_event_handler(int event) {
  int retval;

  switch (event) {
  case NRF_MODEM_GNSS_EVT_PVT:
    retval = nrf_modem_gnss_read(&last_pvt, sizeof(last_pvt), NRF_MODEM_GNSS_DATA_PVT);

    Location location;
    location.lat = last_pvt.latitude;
    location.lon = last_pvt.longitude;

    auto time = last_pvt.datetime;

    // printk("Latitude: %d Longitude: %d\r\n", location.lat, location.lon);

    if (retval == 0) {
      k_sem_give(&pvt_data_sem);
    }
    break;

  case NRF_MODEM_GNSS_EVT_NMEA:
    nrf_modem_gnss_nmea_data_frame* nmea_data = new nrf_modem_gnss_nmea_data_frame;

    retval = nrf_modem_gnss_read(
      nmea_data,
      sizeof(nrf_modem_gnss_nmea_data_frame),
      NRF_MODEM_GNSS_DATA_NMEA
    );

    if (retval == 0) {
      retval = k_msgq_put(&nmea_queue, nmea_data, K_NO_WAIT);
    }

    if (retval != 0) {
      delete nmea_data;
    }

    break;

  case NRF_MODEM_GNSS_EVT_AGNSS_REQ:
    break;

  default:
    break;
  }
}