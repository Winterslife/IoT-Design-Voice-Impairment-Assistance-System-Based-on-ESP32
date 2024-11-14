# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "E:/Developtools/LeXin/espressif/v5.2.1/esp-idf/components/bootloader/subproject"
  "E:/Developtools/LeXin/master_my_project/smart_panel_exchange/build/bootloader"
  "E:/Developtools/LeXin/master_my_project/smart_panel_exchange/build/bootloader-prefix"
  "E:/Developtools/LeXin/master_my_project/smart_panel_exchange/build/bootloader-prefix/tmp"
  "E:/Developtools/LeXin/master_my_project/smart_panel_exchange/build/bootloader-prefix/src/bootloader-stamp"
  "E:/Developtools/LeXin/master_my_project/smart_panel_exchange/build/bootloader-prefix/src"
  "E:/Developtools/LeXin/master_my_project/smart_panel_exchange/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "E:/Developtools/LeXin/master_my_project/smart_panel_exchange/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "E:/Developtools/LeXin/master_my_project/smart_panel_exchange/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
