INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR} ${ROSS_BINARY_DIR})
## Must include binary dir for config.h

## If you will be using the RIO library add this
#INCLUDE_DIRECTORIES(${RIO_SOURCE_DIR} ${RIO_BINARY_DIR})

## Model Version
LIST(APPEND CMAKE_MODULE_PATH "${ROSS_SOURCE_DIR}/cmake/")
INCLUDE(GetGitRevisionDescription)
GET_GIT_HEAD_REVISION(GIT_RESPEC_MODEL GIT_SHA1_MODEL)

## Build Configuration Header File
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/src/highlife_config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/highlife_config.h @ONLY)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/src/highlife_config.h.in PROPERTIES GENERATED FALSE)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/highlife_config.h PROPERTIES GENERATED TRUE)

# All source files for the model
SET(highlife_srcs
	src/driver.c
	src/mapping.c
	src/utils.c
	src/highlife.main.c
)

# Set the executable name here: highlife
# You will be executing the ./highlife binary
ADD_EXECUTABLE(highlife ${highlife_srcs})

TARGET_LINK_LIBRARIES(highlife ROSS m)
## Alternatively, use this command with RIO
#TARGET_LINK_LIBRARIES(model ROSS RIO m)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/highlife DESTINATION bin
  PERMISSIONS
  OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

# Setting optimizations
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mtune=native \
-Wall -Wextra -Wpedantic \
-Wformat=2 -Wno-unused-parameter -Wshadow \
-Wwrite-strings -Wold-style-definition \
-Wnested-externs -Wmissing-include-dirs")
#-Wstrict-prototypes -Wredundant-decls \
