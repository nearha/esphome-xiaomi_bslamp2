#pragma once

#include "esphome/components/gpio/output/gpio_binary_output.h"
#include "esphome/components/ledc/ledc_output.h"
#include "esphome/core/component.h"

namespace esphome {
namespace xiaomi {
namespace bslamp2 {

static const std::string LIGHT_MODE_UNKNOWN{"unknown"};
static const std::string LIGHT_MODE_OFF{"off"};
static const std::string LIGHT_MODE_RGB{"rgb"};
static const std::string LIGHT_MODE_WHITE{"white"};
static const std::string LIGHT_MODE_NIGHT{"night"};

class GPIOOutputValues {
 public:
  float red = 0.0f;
  float green = 0.0f;
  float blue = 0.0f;
  float white = 0.0f;
  std::string light_mode = LIGHT_MODE_OFF;

  /**
   * Copies the current output values to another GPIOOutputValues object.
   */
  void copy_to(GPIOOutputValues *other) {
    other->red = red;
    other->green = green;
    other->blue = blue;
    other->white = white;
    other->light_mode = light_mode;
  }

  void log(const char *prefix) { ESP_LOGD(TAG, "%s: RGB=[%f,%f,%f], white=%f", prefix, red, green, blue, white); }
};

class LightHAL : public Component, public GPIOOutputValues {
 public:
  void set_red_pin(ledc::LEDCOutput *pin) { red_pin_ = pin; }
  void set_green_pin(ledc::LEDCOutput *pin) { green_pin_ = pin; }
  void set_blue_pin(ledc::LEDCOutput *pin) { blue_pin_ = pin; }
  void set_white_pin(ledc::LEDCOutput *pin) { white_pin_ = pin; }
  void set_master1_pin(gpio::GPIOBinaryOutput *pin) { master1_pin_ = pin; }
  void set_master2_pin(gpio::GPIOBinaryOutput *pin) { master2_pin_ = pin; }

  /**
   * Turn on the master switch for the LEDs.
   */
  void turn_on() {
    master1_pin_->turn_on();
    master2_pin_->turn_on();
    is_on_ = true;
  }

  /**
   * Turn off the master switch for the LEDs.
   */
  void turn_off() {
    master1_pin_->turn_off();
    master2_pin_->turn_off();
    is_on_ = false;
  }

  /**
   * Check if the light is turned on.
   */
  bool is_on() {
    return is_on_;
  }

  void set_state(GPIOOutputValues *new_state) {
    new_state->copy_to(this);
    apply_scaled_output_(this->red, this->green, this->blue, this->white);
  }

  void set_rgbw(float r, float g, float b, float w) {
    apply_scaled_output_(r, g, b, w);

    this->red = r;
    this->green = g;
    this->blue = b;
    this->white = w;
  }

  void set_light_mode(std::string light_mode) {
    this->light_mode = light_mode;
  }

 protected:
  // Faixa útil que você gostou no teste.
  // O ponto mais forte do estado atual será remapeado para dentro dessa faixa.
  static constexpr float OUTPUT_MIN = 0.06f;
  static constexpr float OUTPUT_MAX = 0.25f;

  // RGB neste hardware é invertido:
  //   1.0 = apagado
  //   0.0 = máximo
  //
  // White é direto:
  //   0.0 = apagado
  //   1.0 = máximo
  //
  // Para preservar a calibração original, escalamos TODOS os canais pelo
  // MESMO fator, baseado no canal mais forte do estado atual.
  void apply_scaled_output_(float r, float g, float b, float w) {
    float red_emitted = 1.0f - r;
    float green_emitted = 1.0f - g;
    float blue_emitted = 1.0f - b;
    float white_emitted = w;

    float peak = red_emitted;
    if (green_emitted > peak) peak = green_emitted;
    if (blue_emitted > peak) peak = blue_emitted;
    if (white_emitted > peak) peak = white_emitted;

    // Tudo apagado
    if (peak <= 0.0f) {
      red_pin_->set_level(1.0f);
      green_pin_->set_level(1.0f);
      blue_pin_->set_level(1.0f);
      white_pin_->set_level(0.0f);
      return;
    }

    // Remapeia o pico original para a janela 6% -> 25%.
    float target_peak = OUTPUT_MIN + ((OUTPUT_MAX - OUTPUT_MIN) * peak);

    // Mesmo fator para todos os canais => preserva proporções / calibração.
    float factor = target_peak / peak;

    float red_scaled = red_emitted * factor;
    float green_scaled = green_emitted * factor;
    float blue_scaled = blue_emitted * factor;
    float white_scaled = white_emitted * factor;

    // Volta para o formato exigido pelos pinos.
    red_pin_->set_level(1.0f - red_scaled);
    green_pin_->set_level(1.0f - green_scaled);
    blue_pin_->set_level(1.0f - blue_scaled);
    white_pin_->set_level(white_scaled);
  }

  bool is_on_{false};
  ledc::LEDCOutput *red_pin_;
  ledc::LEDCOutput *green_pin_;
  ledc::LEDCOutput *blue_pin_;
  ledc::LEDCOutput *white_pin_;
  gpio::GPIOBinaryOutput *master1_pin_;
  gpio::GPIOBinaryOutput *master2_pin_;
};

}  // namespace bslamp2
}  // namespace xiaomi
}  // namespace esphome
