IF(NOT WIN32)
  STRING(ASCII 27 Esc)
  SET(ColorReset "${Esc}[m")
  SET(ColorBold  "${Esc}[1m")
  SET(Red         "${Esc}[31m")
  SET(Green       "${Esc}[32m")
  SET(Yellow      "${Esc}[33m")
  SET(Blue        "${Esc}[34m")
  SET(Magenta     "${Esc}[35m")
  SET(Cyan        "${Esc}[36m")
  SET(White       "${Esc}[37m")
  SET(BoldRed     "${Esc}[1;31m")
  SET(BoldGreen   "${Esc}[1;32m")
  SET(BoldYellow  "${Esc}[1;33m")
  SET(BoldBlue    "${Esc}[1;34m")
  SET(BoldMagenta "${Esc}[1;35m")
  SET(BoldCyan    "${Esc}[1;36m")
  SET(BoldWhite   "${Esc}[1;37m")
ENDIF()

FUNCTION(read_sdkconfig_value KEY OUT_VAR)
    file(STRINGS "sdkconfig" CONFIG_LINES)
    FOREACH(LINE ${CONFIG_LINES})
        IF(LINE MATCHES "^${KEY}=(.*)$")
            string(REGEX REPLACE "\"" "" VALUE "${CMAKE_MATCH_1}")
            set(${OUT_VAR} ${VALUE} PARENT_SCOPE)
            RETURN()
        ENDIF()
    ENDFOREACH()
    set(${OUT_VAR} "" PARENT_SCOPE)
ENDFUNCTION()

FUNCTION(get_git_info)
    EXECUTE_PROCESS(COMMAND git log --pretty=format:'%h' -n 1 OUTPUT_VARIABLE GIT_REV ERROR_QUIET)
    # Check whether we got any revision (which isn't
    # always the case, e.g. when someone downloaded a zip
    # file from Github instead of a checkout)
    IF("${GIT_REV}" STREQUAL "")
        SET(GIT_REV "NA" PARENT_SCOPE)
        SET(GIT_DIFF "" PARENT_SCOPE)
        SET(GIT_TAG "NA" PARENT_SCOPE)
        SET(GIT_BRANCH "NA" PARENT_SCOPE)
    ELSE()
        EXECUTE_PROCESS(
            COMMAND bash -c "git diff --quiet --exit-code || echo +"
            OUTPUT_VARIABLE GIT_DIFF)
        EXECUTE_PROCESS(
            COMMAND git describe --exact-match --tags
            OUTPUT_VARIABLE GIT_TAG ERROR_QUIET)
        EXECUTE_PROCESS(
            COMMAND git rev-parse --abbrev-ref HEAD
            OUTPUT_VARIABLE GIT_BRANCH)

        STRING(STRIP "${GIT_REV}" GIT_REV)
        STRING(SUBSTRING "${GIT_REV}" 1 7 GIT_REV)
        STRING(STRIP "${GIT_DIFF}" GIT_DIFF)
        STRING(STRIP "${GIT_TAG}" GIT_TAG)
        STRING(STRIP "${GIT_BRANCH}" GIT_BRANCH)

        SET(GIT_REV "${GIT_REV}" PARENT_SCOPE)
        SET(GIT_DIFF "${GIT_DIFF}" PARENT_SCOPE)
        SET(GIT_TAG "${GIT_TAG}" PARENT_SCOPE)

        SET(GIT_BRANCH "${GIT_BRANCH}" PARENT_SCOPE)
        
    ENDIF()
ENDFUNCTION()

FUNCTION(add_project_version_info)
    FILE(READ "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_MAIN}/version.txt" VERSION_STRING)
    # Remove whitespace characters (like newline):
    STRING(STRIP ${VERSION_STRING} VERSION_STRING)
    # Match beginning three numbers (including all dots) and save it as VER_NUM:
    STRING(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)\\." VER_NUM ${VERSION_STRING})
    SET(PROJECT_VERSION_MAJOR "${CMAKE_MATCH_1}")
    SET(PROJECT_VERSION_MINOR "${CMAKE_MATCH_2}")
    SET(PROJECT_VERSION_PATCH "${CMAKE_MATCH_3}")
    # Match last number and save it as BUILD_NUM:
    STRING(REGEX MATCHALL "([0-9]+)$" BUILD_NUM ${VERSION_STRING})

    SET(NO_BUMP_VERSION $ENV{NO_BUMP_VERSION})
    IF(NOT NO_BUMP_VERSION)
        # Increase build number:
        MATH(EXPR BUILD_NUM "${BUILD_NUM}+1")
    ENDIF()
    SET(PROJECT_VERSION_TWEAK "${BUILD_NUM}")

    get_git_info()
    SET(GIT_BRANCH "${GIT_BRANCH}" PARENT_SCOPE)
    CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_MAIN}/version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_MAIN}/version.h)

    # Concatenate back to one string:
    STRING(CONCAT VERSION_STRING ${VER_NUM} ${BUILD_NUM})

    SET(VERSION_OUT ${VERSION_STRING}-${GIT_REV} PARENT_SCOPE)
    # Print info and save to global version file so that esp-idf recognize it:
    MESSAGE("New Project Version: ${BoldYellow}V${VERSION_STRING}${ColorReset}")
    FILE(WRITE "version.txt" ${VERSION_STRING})
    # also save to project directory
    FILE(WRITE "${PROJECT_MAIN}/version.txt" ${VERSION_STRING})

ENDFUNCTION()
