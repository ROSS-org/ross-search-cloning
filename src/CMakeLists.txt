# Setting up ROSS and additional utilities (Git stuff is used in
# configuration header file, below)
include_directories(
  "${ROSS_SOURCE_DIR}"
  "${ROSS_BINARY_DIR}"
)
# Uncomment in case of using RIO
#include_directories(${RIO_SOURCE_DIR} ${RIO_BINARY_DIR})
list(APPEND CMAKE_MODULE_PATH "${ROSS_SOURCE_DIR}/cmake/")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_RESPEC_MODEL GIT_SHA1_MODEL)

# Building configuration header file
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_NAME}_config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/${MODEL_NAME}_config.h
  @ONLY
)
set_source_files_properties(
  ${CMAKE_CURRENT_BINARY_DIR}/${MODEL_NAME}_config.h
  PROPERTIES GENERATED TRUE
)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Compiling ROSS model (${MODEL_NAME})
set(model_srcs
  driver.c
  mapping.c
  utils.c
  ${MODEL_NAME}.main.c
)

add_executable(${MODEL_NAME} ${model_srcs})
target_link_libraries(${MODEL_NAME}
  PRIVATE
    m
    ROSS
#   RIO # uncomment if using RIO
)

# Setting optimizations
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mtune=native \
-Wall -Wextra -Wpedantic \
-Wformat=2 -Wno-unused-parameter -Wshadow \
-Wwrite-strings -Wold-style-definition \
-Wnested-externs -Wmissing-include-dirs")
#-Wstrict-prototypes -Wredundant-decls

# Including variables that indicate where the usr/bin is.
# It defines ${CMAKE_INSTALL_FULL_BINDIR}
include(GNUInstallDirs)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${MODEL_NAME}
  DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
  PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
)
