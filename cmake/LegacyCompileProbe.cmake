# Compile the complete historical C++/SWIG surface before creating a game executable.
# Audio is intentionally absent from this compile-only probe; no silent game is shipped.
find_package(Qt6Core5Compat 6.11.1 EXACT CONFIG REQUIRED)
find_package(Qt6 6.11.1 EXACT REQUIRED COMPONENTS Widgets Network Sql)
find_package(SWIG 4.4.1 EXACT REQUIRED)
file(GLOB SWIG_INPUTS CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/swig/*.i")
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sanguosha_wrap.cxx"
  COMMAND ${SWIG_EXECUTABLE} -c++ -lua -I${PROJECT_SOURCE_DIR}/swig
    -o "${CMAKE_CURRENT_BINARY_DIR}/sanguosha_wrap.cxx" "${PROJECT_SOURCE_DIR}/swig/sanguosha.i"
  DEPENDS ${SWIG_INPUTS} VERBATIM)
set(LEGACY_SOURCES)
set(LEGACY_INCLUDES)
foreach(dir client core dialog package scenario server ui util)
  file(GLOB part CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/${dir}/*.cpp" "${PROJECT_SOURCE_DIR}/src/${dir}/*.h")
  list(APPEND LEGACY_SOURCES ${part})
  list(APPEND LEGACY_INCLUDES "${PROJECT_SOURCE_DIR}/src/${dir}")
endforeach()
list(FILTER LEGACY_SOURCES EXCLUDE REGEX "/(audio|joystick)\\.(cpp|h)$")
file(GLOB JSON_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/jsoncpp/src/*.cpp")
add_library(qs_legacy_objects OBJECT EXCLUDE_FROM_ALL ${LEGACY_SOURCES} ${JSON_SOURCES}
  "${CMAKE_CURRENT_BINARY_DIR}/sanguosha_wrap.cxx")
set_target_properties(qs_legacy_objects PROPERTIES AUTOUIC ON)
target_include_directories(qs_legacy_objects PRIVATE ${LEGACY_INCLUDES} src/jsoncpp/include include)
target_link_libraries(qs_legacy_objects PRIVATE qs_lua Qt6::Widgets Qt6::Network Qt6::Sql Qt6::Core5Compat)
