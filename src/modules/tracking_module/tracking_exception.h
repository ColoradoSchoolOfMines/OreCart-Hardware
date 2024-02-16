#pragma once
#include <string>

class TrackingException {
  std::string _exception;

public:
  TrackingException(std::string & exception);
  TrackingException(const char * exception);

  const char* toStr() {
    return _exception.c_str();
  }
};
