#pragma once

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#include "../../common/location.h"

struct tracking_event {
  /** Data module application event header. */
  struct app_event_header header;
  /** Data module event type. */
  enum data_module_event_type type;

  union {
    Location location;
  } data;
};