# SPDX-License-Identifier: MIT

# The left half is the central and owns the host Bluetooth profiles.
# The right half only forwards key positions, and it does not link the BLE profile API.
if((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/behaviors/behavior_host_clip.c)
endif()
