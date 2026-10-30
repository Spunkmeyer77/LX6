# LX-6: copyright, licence and trademark notice

LX-6 plugin code (everything outside `engine/lxr`): Copyright (C) 2026 Ticco Ross, licensed under the
GNU General Public License version 3 (GPL-3.0-only). See [LICENSE](LICENSE).

## LXR firmware

`engine/lxr` contains source code from the Erica Synths LXR desktop firmware
(https://github.com/erica-synths/lxr-desktop-app), Copyright (C) 2012-2026 Erica Synths, licensed under
GPL-3.0-only with the additional permission and additional terms in [engine/lxr/NOTICE.md](engine/lxr/NOTICE.md).

This is a **modified version**. The changes:

- the hardware layer (codec, DMA, front panel, LCD, LEDs, SD card, USB, UART) is not used; `engine/host`
  replaces it with stubs and a C interface for the plugin;
- `LX6_HOST` sections make the code safe on a desktop computer: no TCM memory sections, portable versions of
  ARM DSP instructions, bounds-safe table and delay-buffer access where the hardware relied on reading
  neighbouring memory, and the output jacks reported as always connected;
- `engine/lxr/Src/Cc2Text.c` (the modulation-target table) is taken from the firmware's menu code.

As required by the firmware's additional terms, LX-6 is named and presented as a different product. It is
not official Erica Synths firmware or software.

## Trademarks

"Erica Synths", "LXR" and "LXR-02" are trademarks of Erica Synths. The GPL does not grant any rights to
use them. LX-6 uses them only in truthful statements of origin and compatibility (for example "based on
the open-source Erica Synths LXR firmware" and "editor for the Erica Synths LXR-02"). LX-6 is not made,
approved or endorsed by Erica Synths. The LXR hardware and its panel design are not part of LX-6.

## Third-party components

| Component | Licence |
|---|---|
| JUCE (downloaded at build time) | AGPLv3 / commercial; combined here under GPL-3.0 section 13 |
| Space Grotesk font (Florian Karsten) | SIL Open Font License 1.1, `Resources/fonts/OFL-SpaceGrotesk.txt` |
| JetBrains Mono font (JetBrains) | SIL Open Font License 1.1, `Resources/fonts/OFL-JetBrainsMono.txt` |
| STM32 HAL configuration header in `engine/lxr/Inc` (STMicroelectronics) | BSD-3-Clause, as stated in the file |
