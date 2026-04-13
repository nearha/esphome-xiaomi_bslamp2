#pragma once

#include <array>
#include <stdexcept>

#include "../common.h"
#include "color_handler.h"
#include "color_handler_off.h"
#include "color_handler_rgb.h"
#include "color_handler_color_temperature.h"

namespace esphome {
namespace xiaomi {
namespace bslamp2 {

/**
 * This class translates LightColorValues into GPIO duty cycles that can be
 * used for representing a requested light color on the physical device.
 *
 * In this fork, night light mode is disabled.
 *
 * The code handles:
 * - off
 * - white light: based on color temperature + brightness
 * - RGB light: based on RGB values + brightness
 */
class ColorHandlerChain : public ColorHandler {
 public:
  bool set_light_color_values(light::LightColorValues v) {
    if (off_light_.set_light_color_values(v))
      off_light_.copy_to(this);
    else if (white_light_.set_light_color_values(v))
      white_light_.copy_to(this);
    else if (rgb_light_.set_light_color_values(v))
      rgb_light_.copy_to(this);
    else {
      ESP_LOGE(TAG, "Light color error: (None of the ColorHandler classes handles the requested light state; defaulting to 'off')");
      off_light_.copy_to(this);
    }

    return true;
  }

 protected:
  ColorHandlerOff off_light_;
  ColorHandlerRGB rgb_light_;
  ColorHandlerColorTemperature white_light_;
};

}  // namespace bslamp2
}  // namespace xiaomi
}  // namespace esphome
