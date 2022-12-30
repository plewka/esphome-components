#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/i2c/i2c.h"

// TCA6408A is derived from PCA9557
// Difference: 
//  - no special P0 (PushPull here)
//  - has IRQ output
//  - output register different default (11111111 here)
//  - input polarity register different default (00000000 here)
//  - address 0x20-0x21

namespace esphome {
namespace tca6408a {

class TCA6408AComponent : public Component, public i2c::I2CDevice {
 public:
  TCA6408AComponent() = default;

  /// Check i2c availability and setup masks
  void setup() override;
  /// Helper function to read the value of a pin.
  bool digital_read(uint8_t pin);
  /// Helper function to write the value of a pin.
  void digital_write(uint8_t pin, bool value);
  /// Helper function to set the pin mode of a pin.
  void pin_mode(uint8_t pin, gpio::Flags flags);

  float get_setup_priority() const override;

  void dump_config() override;

 protected:
  bool read_gpio_();

  bool write_gpio_();

  /// Mask for the pin mode - 1 means input, 0 means output
  uint16_t mode_mask_{0x00};
  /// The mask to write as output state - 1 means HIGH, 0 means LOW
  uint16_t output_mask_{0x00};
  /// The state read in read_gpio_ - 1 means HIGH, 0 means LOW
  uint16_t input_mask_{0x00};
  uint16_t ignore_;
};

/// Helper class to expose a TCA6408A pin as an internal input GPIO pin.
class TCA6408AGPIOPin : public GPIOPin {
 public:
  void setup() override;
  void pin_mode(gpio::Flags flags) override;
  bool digital_read() override;
  void digital_write(bool value) override;
  std::string dump_summary() const override;

  void set_parent(TCA6408AComponent *parent) { parent_ = parent; }
  void set_pin(uint8_t pin) { pin_ = pin; }
  void set_inverted(bool inverted) { inverted_ = inverted; }
  void set_flags(gpio::Flags flags) { flags_ = flags; }
  void set_state(bool state) { default_state_ = state; }

 protected:
  TCA6408AComponent *parent_;
  uint8_t pin_;
  bool inverted_;
  bool default_state_;
  gpio::Flags flags_;
};

}  // namespace tca6408a
}  // namespace esphome
