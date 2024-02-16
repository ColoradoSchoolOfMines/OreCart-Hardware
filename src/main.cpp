#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include <app_event_manager.h>

#include "common/van_info.h"
#include "common/location.h"

#include "modules/net_module/net_module.h"
#include "modules/tracking_module/tracking_module.h"
#include "modules/tracking_module/tracking_event.h"
#include "modules/tracking_module/tracking_exception.h"

VanInfo van_info;

int main() {
  OC_LOG_INFO("Welcome to the OreCart Hardware Project!");
  int ret;
  
  net_module_init();

  try {
    tracking_module_init();
  } catch (TrackingException e) {
    OC_LOG_ERROR(e.toStr());
  }

  while (true) {
    k_sleep(K_SECONDS(1));
  }

  return 0;
}

static bool _app_event_handler(const struct app_event_header *aeh) {
  if (is_tracking_event(aeh)) {
    
  }
}

APP_EVENT_LISTENER(main, _app_event_handler);
APP_EVENT_SUBSCRIBE(main, tracking_event);