#pragma once

#include <sstream>

namespace raw {
  struct ErrorMessage {
    bool used = false;
    std::stringstream ss;

    template<typename T>
    ErrorMessage& operator<<(const T &data) {
      used = true;
      ss << data;
      return *this;
    }
    
    operator std::string() {
      return ss.str();
    }
  };
}
