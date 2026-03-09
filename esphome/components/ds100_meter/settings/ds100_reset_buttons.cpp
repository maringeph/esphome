#include "ds100_reset_buttons.h"

#ifdef USE_BUTTON

#include "../ds100_meter.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ds100_meter {

static const char *const TAG = "ds100_meter.button";

void DS100ResetMaximumDemandButton::press_action() {
  ESP_LOGI(TAG, "Resetting maximum demand");
  this->parent_->reset_maximum_demand();
}

void DS100ResetStatisticsButton::press_action() {
  ESP_LOGI(TAG, "Resetting statistics");
  this->parent_->reset_statistics();
}

}  // namespace ds100_meter
}  // namespace esphome

#endif  // USE_BUTTON
