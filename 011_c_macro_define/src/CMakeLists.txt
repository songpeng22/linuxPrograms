CMAKE_MINIMUM_REQUIRED(VERSION 3.2)
message( "****************************************break line****************************************" )
message( "****************************************APITest/src****************************************" )
MESSAGE(STATUS "CMAKE_GENERATOR: " ${CMAKE_GENERATOR})
MESSAGE(STATUS "CMAKE_SOURCE_DIR: " ${CMAKE_SOURCE_DIR})
MESSAGE(STATUS "CMAKE_CURRENT_BINARY_DIR: " ${CMAKE_CURRENT_BINARY_DIR})

#Solution name
PROJECT(build_APITest)
#project name/target name
SET ( TARGET_NAME APITest )

# CMake define
# CMAKE_VERBOSE_MAKEFILE -> enable make output verbose
set(CMAKE_VERBOSE_MAKEFILE ON) # or make VERBOSE=1

# make sure that the Android toolchain is used
IF(NOT ANDROID)
    MESSAGE(STATUS "ANDROID not exist.")
ELSE()
    MESSAGE(STATUS "ANDROID: " ${ANDROID})
ENDIF()

# CMake build flags: defined in android_predefine.cmake
# add addtional flag here
add_compile_options(-Wall)
MESSAGE(STATUS "CMAKE_C_FLAGS: " ${CMAKE_C_FLAGS})
MESSAGE(STATUS "CMAKE_CXX_FLAGS: " ${CMAKE_CXX_FLAGS})
MESSAGE(STATUS "CMAKE_C_FLAGS_DEBUG: " ${CMAKE_C_FLAGS_DEBUG})
MESSAGE(STATUS "CMAKE_CXX_FLAGS_DEBUG: " ${CMAKE_CXX_FLAGS_DEBUG})

link_directories(
    #../../android/libs/
)

#project
FILE (GLOB PROJECT_SOURCES
    *.cpp

)

SET (PROJECT_HEADERS
    
)

ADD_EXECUTABLE( ${TARGET_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS} )

target_include_directories( ${TARGET_NAME}
    PRIVATE ../include
)

# Platform libs
set(PLIBS
    #liblog.so
)

# Must After add_executable
target_link_libraries( ${TARGET_NAME}
    ${PLIBS}
)

    #add_custom_command(TARGET <target>
    #               PRE_BUILD | PRE_LINK | POST_BUILD
    #               COMMAND command1 [ARGS] [args1...]
    #               [COMMAND command2 [ARGS] [args2...] ...]
    #               [BYPRODUCTS [files...]]
    #               [WORKING_DIRECTORY dir]
    #               [COMMENT comment]
    #               [VERBATIM] [USES_TERMINAL]
    #               [COMMAND_EXPAND_LISTS])
    add_custom_command(
        # after the target has been created, this custom command will start to run
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "post build of build_library:.............................................................................1"
        #COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../../lib${TARGET_NAME}/src/lib${TARGET_NAME}.so ${CMAKE_SOURCE_DIR}/android/libs/
        #COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/libTest1/src/libTest1.h ${CMAKE_SOURCE_DIR}/android/include/
    )


