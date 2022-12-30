#include "tca6408a.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tca6408a {

static const char *const TAG = "tca6408a";

void TCA6408AComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up TCA6408A...");
  if (!this->read_gpio_()) {
    ESP_LOGE(TAG, "TCA6408A not available under 0x%02X", this->address_);
    this->mark_failed();
    return;
  }

  this->ignore_ = 100;

  this->write_gpio_();
  this->read_gpio_();
}
void TCA6408AComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "TCA6408A:");
  LOG_I2C_DEVICE(this)
  //ESP_LOGCONFIG(TAG, "  Is PCF8575: %s", YESNO(this->pcf8575_));
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with TCA6408A failed!");
  }
}
bool TCA6408AComponent::digital_read(uint8_t pin) {
  if(this->ignore_)
    this->ignore_--;
  else{
    this->read_gpio_();
    this->ignore_ = 100;
  }
  return this->input_mask_ & (1 << pin);
}
void TCA6408AComponent::digital_write(uint8_t pin, bool value) {

  if(((this->output_mask_ & (1 << pin)) >> pin) == value){
    //ESP_LOGD(TAG, "equal");
  }else{

    uint8_t data[2];
    this->read_register(0x1, data, 1);

    this->output_mask_ = data[0];

    if (value) {
      this->output_mask_ |= (1 << pin);
    } else {
      this->output_mask_ &= ~(1 << pin);
    }

    this->write_gpio_();
  }
}
void TCA6408AComponent::pin_mode(uint8_t pin, gpio::Flags flags) {

  uint8_t data[2];

  this->read_register(0x3, data, 1);
  this->mode_mask_ = data[0];

  if (flags == gpio::FLAG_INPUT) {
    // Clear mode mask bit
    this->mode_mask_ |= (1 << pin);
    // Write GPIO to enable input mode
    this->write_gpio_();
  } else if (flags == gpio::FLAG_OUTPUT) {
    // Set mode mask bit
    this->mode_mask_ &= ~(1 << pin);
  }


  data[0] = this->mode_mask_;
  data[1] = 0;//value >> 8;

  if (this->write_register(0x03, data, 1) != i2c::ERROR_OK) {
    this->status_set_warning();
    //return false;
  }

  //ESP_LOGD(TAG, "Mode");
  //ESP_LOGD(TAG, "Input: %X", this->input_mask_);
  //ESP_LOGD(TAG, "Output: %X", this->output_mask_);
  //ESP_LOGD(TAG, "Mode: %X", this->mode_mask_);
}
bool TCA6408AComponent::read_gpio_() {
  if (this->is_failed())
    return false;
  int success;
  uint8_t data[2];
  //if (this->pcf8575_) {
  //  success = this->read_bytes_raw(data, 2);
  //  this->input_mask_ = (uint16_t(data[1]) << 8) | (uint16_t(data[0]) << 0);
  //} else {
    success = this->read_register(0x0, data, 1);
    this->input_mask_ = data[0];
  //}

  if (success != 0) {
    this->status_set_warning();
    return false;
  }
  this->status_clear_warning();

  //ESP_LOGD(TAG, "Read");
  //ESP_LOGD(TAG, "Input: %X", this->input_mask_);
  //ESP_LOGD(TAG, "Output: %X", this->output_mask_);
  //ESP_LOGD(TAG, "Mode: %X", this->mode_mask_);


  return true;
}
bool TCA6408AComponent::write_gpio_() {
  if (this->is_failed())
    return false;

  /*uint16_t value = 0;
  // Pins in OUTPUT mode and where pin is HIGH.
  value |= this->mode_mask_ & this->output_mask_;
  // Pins in INPUT mode must also be set here
  value |= ~this->mode_mask_;*/

  uint8_t data[2];

  //success = this->read_register(0x1, data, 1);

  //uint8_t value = 0;
  //value = data[0];



  data[0] = this->output_mask_;
  //data[1] = value >> 8;

  if (this->write_register(0x01, data, 1) != i2c::ERROR_OK) {
    this->status_set_warning();
    return false;
  }

  this->status_clear_warning();

  //ESP_LOGD(TAG, "Write");
  //ESP_LOGD(TAG, "Input: %X", this->input_mask_);
  //ESP_LOGD(TAG, "Output: %X", this->output_mask_);
  //ESP_LOGD(TAG, "Mode: %X", this->mode_mask_);
  return true;
}
float TCA6408AComponent::get_setup_priority() const { return setup_priority::IO; }

void TCA6408AGPIOPin::setup() { pin_mode(flags_); digital_write(default_state_); }
void TCA6408AGPIOPin::pin_mode(gpio::Flags flags) { this->parent_->pin_mode(this->pin_, flags); }
bool TCA6408AGPIOPin::digital_read() { return this->parent_->digital_read(this->pin_) != this->inverted_; }
void TCA6408AGPIOPin::digital_write(bool value) { this->parent_->digital_write(this->pin_, value != this->inverted_); }
std::string TCA6408AGPIOPin::dump_summary() const {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%u via TCA6408A", pin_);
  return buffer;
}

}  // namespace tca6408a
}  // namespace esphome
