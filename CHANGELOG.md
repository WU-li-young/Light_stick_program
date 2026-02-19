# 📝 Changelog

This document records all notable updates, new features, and bug fixes for the "ESP32-C3 Dual-Head Light Stick Control System" across every version.

## [5.0.1] - Mobile Scrolling Fix

### Fixed

* **UI Scrolling**: Removed the `touch-action: none` attribute from the webpage's `body` to restore vertical scrolling on mobile browsers.
* **Canvas Interaction**: Applied `touch-action: none` specifically to the color wheel canvas to ensure smooth color picking without accidentally dragging the screen.

## [5.0.0] - Single-Pin Parallel Architecture

### Hardware & Logic

* **Pin Unification**: Optimized the hardware configuration to output all signals through a single data pin for parallel strip wiring setups.

## [4.0.0] - Dual-Pin Experimental Build

### Added

* **Hardware Split**: Introduced independent dual-pin output functionality. The TOP signal was mapped to GPIO 8, and the BTM signal was mapped to GPIO 10, utilizing separate `FastLED.addLeds` initializations.

## [3.0.5] - UI Alignment Update

### Changed

* **Preview Box Layout**: Adjusted the dimensions of the virtual preview blocks on the web interface to better simulate the physical dual-head structure.

## [3.0.4] - POV Effects & UI Overhaul

### Added

* **POV (Persistence of Vision) Mode**: Developed high-speed effects specifically designed for spinning and juggling, including Strobe, Segment, Hyper Rainbow, and Fractal Dot.
* **Independent Brightness**: Introduced separate brightness variables (`briA` and `briB`) and control sliders for the TOP and BTM sections.
* **Quick Jump Panel**: Added a quick-select grid of 8 solid colors for instant switching during live performances[cite: 740].

### Changed

* **Tabbed Interface**: Reorganized the control panel into two distinct hidden/active views ("Static/Jump" and "Effects Library") to reduce screen clutter.

## [3.0.3] - Preset Library System

### Added

* **Dropdown Menu**: Replaced the standard effect grid with a comprehensive `<select>` dropdown menu.
* **Themed Presets**: Introduced complex time-based presets including Police (Red/Blue), Christmas (Red/Green), Halloween (Orange/Purple), Fire, Lightning, and Ice.

### Changed

* **Backend Refactoring**: Rewrote the animation loop to utilize a `runPreset` switch-case logic based on `millis()` timing.

## [3.0.2] - Advanced Cycle & Party Modes

### Added

* **6-Color Cycle**: Implemented an automated cycle through Red, Yellow, Green, Blue, Purple, and White using time-based intervals.
* **Party Mode**: Added a high-intensity strobe effect that alternates between random rainbow flashes and solid white bursts.

## [3.0.1] - Hard Cut Animations

### Changed

* **Discrete Color Switching**: Modified the color cycle animation logic (`discreteHue`) in FastLED and the CSS `@keyframes` to snap instantly between colors (Hard Cut) rather than fading smoothly.

## [3.0.0] - Power Stability Update

### Fixed

* **Brownout Protection**: Implemented `WiFi.setTxPower(WIFI_POWER_11dBm)` to forcefully lower the Wi-Fi transmission power. This prevents the ESP32 from crashing or rebooting due to voltage drops when running on battery power at high LED brightness.

## [2.0.4] - Color Update Logic Fix

### Fixed

* **Effect Color Change**: Removed the legacy logic that forced the system back to "Static mode" whenever a color was picked. Users can now change the base color of running effects (like Breathe or Chase) seamlessly.

## [2.0.3] - Speed Initialization

### Changed

* **Fast Boot**: Cleaned up code structure and removed unnecessary fade-in logic during startup to allow the stick to boot up and display effects instantly.

## [2.0.2] - HTML5 Canvas Color Wheel

### Added

* **Circular Color Picker**: Completely replaced the native HTML `input[type=color]` with a custom-drawn HTML5 `<canvas>` circular color wheel for precise hue and saturation selection.

### Fixed

* **Rate Limiting**: Added a `50ms` delay gate (`Date.now() - lastSend > 50`) to the Canvas touch events to prevent rapid swiping from flooding the ESP32 with HTTP requests.

## [2.0.1] - Live CSS Animations

### Added

* **Frontend Sync**: Implemented advanced CSS `@keyframes` (such as `rainbow-anim` and `breathe-anim`) to make the virtual light stick on the webpage mimic the physical LED animations.
* **Math Optimization**: Improved FastLED effect smoothness using functions like `cubicwave8` and `beatsin16`.

## [2.0.0] - FastLED Animation Core

### Added

* **Dynamic Effects**: Upgraded from static-only colors to a dynamic animation engine utilizing `beat8()`. Added Rainbow, Breathe, and Chase effects.
* **Speed Control**: Added a master speed slider to adjust the frequency of the active animations.

## [1.0.2] - Dual-Zone Control Architecture

### Added

* **Sectioned Control**: Divided the logic into TOP and BTM sections (`HALF_LEDS`). Added UI buttons for "SYNC", "TOP", and "BTM" to route commands to specific segments.
* **RGB Sliders**: Introduced independent Red, Green, and Blue numeric sliders for precise color mixing alongside standard hex input.

## [1.0.1] - Visual Feedback Enhancement

### Added

* **Virtual Preview Bulb**: Added a circular preview box (`#preview-box`) to the web interface that dynamically changes its background color and emits a CSS `box-shadow` glow based on the user's color selection.

## [1.0.0] - Project Inception

### Added

* **Basic Infrastructure**: Established the ESP32 Wi-Fi Access Point and WebServer routing.
* **Core Functions**: Integrated the FastLED library to provide basic full-stick static color selection, global brightness control, and a power-off button.
