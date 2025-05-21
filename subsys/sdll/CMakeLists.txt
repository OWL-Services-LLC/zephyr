# SPDX-License-Identifier: Apache-2.0

if (CONFIG_SDLL)
  zephyr_library()
  zephyr_library_sources(sdll.c)
endif()