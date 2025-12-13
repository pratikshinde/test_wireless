# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "bootloader\\bootloader.bin"
  "bootloader\\bootloader.elf"
  "bootloader\\bootloader.map"
  "config\\sdkconfig.cmake"
  "config\\sdkconfig.h"
  "dashboard.html.S"
  "devices.html.S"
  "esp-idf\\esptool_py\\flasher_args.json.in"
  "esp-idf\\mbedtls\\x509_crt_bundle"
  "flash_app_args"
  "flash_bootloader_args"
  "flash_project_args"
  "flasher_args.json"
  "ldgen_libraries"
  "ldgen_libraries.in"
  "lora_config.html.S"
  "lora_mesh_1.bin"
  "lora_mesh_1.map"
  "project_elf_src_esp32.c"
  "script.js.S"
  "styles.css.S"
  "x509_crt_bundle.S"
  )
endif()
