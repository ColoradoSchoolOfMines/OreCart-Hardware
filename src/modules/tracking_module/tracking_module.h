#pragma once

#include "../../common/location.h"

void tracking_module_init();

Location location_gnss_high_accuracy_get();

void location_with_fallback_get();