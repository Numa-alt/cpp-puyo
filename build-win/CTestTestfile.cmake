# CMake generated Testfile for 
# Source directory: C:/Users/Numashita/cpp-study/cpp-puyo
# Build directory: C:/Users/Numashita/cpp-study/cpp-puyo/build-win
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[GameEventTest]=] "C:/Users/Numashita/cpp-study/cpp-puyo/build-win/Debug/test_game_event.exe")
  set_tests_properties([=[GameEventTest]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Numashita/cpp-study/cpp-puyo/CMakeLists.txt;64;add_test;C:/Users/Numashita/cpp-study/cpp-puyo/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[GameEventTest]=] "C:/Users/Numashita/cpp-study/cpp-puyo/build-win/Release/test_game_event.exe")
  set_tests_properties([=[GameEventTest]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Numashita/cpp-study/cpp-puyo/CMakeLists.txt;64;add_test;C:/Users/Numashita/cpp-study/cpp-puyo/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[GameEventTest]=] "C:/Users/Numashita/cpp-study/cpp-puyo/build-win/MinSizeRel/test_game_event.exe")
  set_tests_properties([=[GameEventTest]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Numashita/cpp-study/cpp-puyo/CMakeLists.txt;64;add_test;C:/Users/Numashita/cpp-study/cpp-puyo/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[GameEventTest]=] "C:/Users/Numashita/cpp-study/cpp-puyo/build-win/RelWithDebInfo/test_game_event.exe")
  set_tests_properties([=[GameEventTest]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Numashita/cpp-study/cpp-puyo/CMakeLists.txt;64;add_test;C:/Users/Numashita/cpp-study/cpp-puyo/CMakeLists.txt;0;")
else()
  add_test([=[GameEventTest]=] NOT_AVAILABLE)
endif()
