#pragma once
#ifdef USE_SSH_OTA
#include "esphome/components/ota/ota_backend.h"

namespace esphome {
namespace ssh_ota {

// Minimal stub component - SSH OTA is handled entirely in Python
class SSHOTAComponent : public ota::OTAComponent {
 public:
  void setup() override {}
  void dump_config() override {}
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }
};

}  // namespace ssh_ota
}  // namespace esphome
#endif
