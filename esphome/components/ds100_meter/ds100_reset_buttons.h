#pragma once

#ifdef USE_BUTTON

#include "esphome/components/button/button.h"
#include "esphome/core/component.h"

namespace esphome {
namespace ds100_meter {

class DS100Meter;

/// Button to reset maximum demand values
class DS100ResetMaximumDemandButton : public button::Button, public Component {
 public:
  void set_parent(DS100Meter *parent) { this->parent_ = parent; }

 protected:
  void press_action() override;
  DS100Meter *parent_{nullptr};
};

/// Button to reset resettable statistics (energy counters)
class DS100ResetStatisticsButton : public button::Button, public Component {
 public:
  void set_parent(DS100Meter *parent) { this->parent_ = parent; }

 protected:
  void press_action() override;
  DS100Meter *parent_{nullptr};
};

}  // namespace ds100_meter
}  // namespace esphome

#endif  // USE_BUTTON
