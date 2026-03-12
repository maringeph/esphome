#include "ds100_settings_number.h"
#include "ds100_meter.h"

// Template instantiation requires full DS100Meter definition
namespace esphome {
namespace ds100_meter {
// Explicit instantiations for the template classes
// This ensures the template methods are compiled with full DS100Meter definition
template class DS100RegisterNumber<0x1003, 1, 247>;
template class DS100RegisterNumber<0x100B, 0, 99>;
template class DS100RegisterNumber<0x1011, 1, 30>;
template class DS100RegisterNumber<0x1016, 0, 9999>;
}  // namespace ds100_meter
}  // namespace esphome
