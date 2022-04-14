cmake_minimum_required (VERSION 3.13.4)
include (GenerateExportHeader)
project (Orkid)

################################################################################

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)

################################################################################

set(CMAKE_INSTALL_RPATH "$ENV{OBT_STAGE}/lib")
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)

################################################################################

set(CMAKE_FIND_DEBUG_MODE OFF)
set(PYTHON_EXECUTABLE $ENV{PYTHONHOME}/bin/python3)
set(PYTHON_LIBRARY $ENV{PYTHONHOME}/lib/libpython3.9d.so)

set(Python3_FIND_STRATEGY "LOCATION")
set(Python3_ROOT_DIR $ENV{PYTHONHOME} )

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)

#############################################################################################################

set( ORKROOT $ENV{ORKID_WORKSPACE_DIR} )
set( ORK_CORE_INCD ${ORKROOT}/ork.core/inc )
set( ORK_LEV2_INCD ${ORKROOT}/ork.lev2/inc )
set( ORK_ECS_INCD ${ORKROOT}/ork.ecs/inc )
set( ORK_ECS_SRCD ${ORKROOT}/ork.ecs/src )

################################################################################

IF(${APPLE})
    #set(MACOSX_DEPLOYMENT_TARGET=11)
    set(XCODE_SDKBASE /Library/Developer/CommandLineTools/SDKs)
    set(CMAKE_OSX_SYSROOT ${XCODE_SDKBASE}/MacOSX10.15.sdk)
    set(CMAKE_MACOSX_RPATH 1)
    LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "$ENV{OBT_STAGE}/lib" isSystemDir)
    IF("${isSystemDir}" STREQUAL "-1")
       SET(CMAKE_INSTALL_RPATH "$ENV{OBT_STAGE}/lib")
    ENDIF("${isSystemDir}" STREQUAL "-1")

    macro(ADD_OSX_FRAMEWORK fwname target)
        find_library(FRAMEWORK_${fwname}
        NAMES ${fwname}
        PATHS ${CMAKE_OSX_SYSROOT}/System/Library
        PATH_SUFFIXES Frameworks
        NO_DEFAULT_PATH)
        if( ${FRAMEWORK_${fwname}} STREQUAL FRAMEWORK_${fwname}-NOTFOUND)
            MESSAGE(ERROR ": Framework ${fwname} not found")
        else()
            TARGET_LINK_LIBRARIES(${target} PUBLIC "${FRAMEWORK_${fwname}}/${fwname}")
            MESSAGE(STATUS "Framework ${fwname} found at ${FRAMEWORK_${fwname}}")
        endif()
    endmacro(ADD_OSX_FRAMEWORK)

    set(CMAKE_MACOSX_RPATH 1)
    LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "$ENV{OBT_STAGE}/lib" isSystemDir)
    IF("${isSystemDir}" STREQUAL "-1")
       SET(CMAKE_INSTALL_RPATH "$ENV{OBT_STAGE}/lib")
    ENDIF("${isSystemDir}" STREQUAL "-1")
ENDIF()

##############################

IF( "${ARCHITECTURE}" STREQUAL "x86_64" )
    add_compile_options(-mavx)
ELSEIF( "${ARCHITECTURE}" STREQUAL "AARCH64" )
ENDIF()

#############################################################################################################

function(ork_std_target_set_incdirs the_target)

  set_property( TARGET ${the_target} APPEND PROPERTY TGT_INCLUDE_PATHS $ENV{OBT_STAGE}/include/eigen3 )
  set_property( TARGET ${the_target} APPEND PROPERTY TGT_INCLUDE_PATHS $ENV{OBT_STAGE}/include)
  set_property( TARGET ${the_target} APPEND PROPERTY TGT_INCLUDE_PATHS $ENV{OBT_STAGE}/include/tuio/oscpack)
  set_property( TARGET ${the_target} APPEND PROPERTY TGT_INCLUDE_PATHS ${Python3_INCLUDE_DIRS} ${PYBIND11_INCLUDE_DIRS} )

  # IGL (its a beast, needs a cmake update)
  IF( "${ARCHITECTURE}" STREQUAL "x86_64" )
    set_property( TARGET ${the_target} APPEND PROPERTY TGT_INCLUDE_PATHS $ENV{OBT_BUILDS}/igl/include )
    set_property( TARGET ${the_target} APPEND PROPERTY TGT_INCLUDE_PATHS $ENV{OBT_BUILDS}/igl/external/triangle )
  ENDIF()

  # use homebrew last
  IF(${APPLE})
    set_property( TARGET ${the_target} APPEND PROPERTY TGT_INCLUDE_PATHS /usr/local/include)
  ENDIF()

endfunction()

#############################################################################################################

function(ork_std_target_set_defs the_target)

  set( def_list "" )

  IF(PROFILER)
    list(APPEND def_list -DBUILD_WITH_EASY_PROFILER)
  ENDIF()

  IF( "${ARCHITECTURE}" STREQUAL "x86_64" )
    list(APPEND def_list -DORK_ARCHITECTURE_X86_64)
  ELSEIF( "${ARCHITECTURE}" STREQUAL "AARCH64" )
    list(APPEND def_list -DORK_ARCHITECTURE_ARM_64)
  ENDIF()

  if(${APPLE})
    list(APPEND def_list -DOSX -DORK_OSX )
  ELSE()
    list(APPEND def_list -DORK_CONFIG_IX -DLINUX -DGCC )
    list(APPEND def_list -D_REENTRANT -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE )
  ENDIF()

  list(SORT def_list)

  set_property( TARGET ${the_target} APPEND PROPERTY TGT_DEFINITIIONS ${def_list} )

endfunction()

#############################################################################################################

function(ork_std_target_set_opts the_target)

  set( opt_list "" )

  list(APPEND opt_list -Wall -Wpedantic)
  list(APPEND opt_list -Wno-deprecated -Wno-register -Wno-switch-enum)
  list(APPEND opt_list -Wno-unused-command-line-argument)
  list(APPEND opt_list -Wno-unused -Wno-extra-semi)
  list(APPEND opt_list -fPIE -fPIC )
  list(APPEND opt_list -fextended-identifiers)
  list(APPEND opt_list -fexceptions)
  list(APPEND opt_list -fvisibility=default)
  list(APPEND opt_list -fno-common -fno-strict-aliasing )
  list(APPEND opt_list -g  )


  IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(APPEND opt_list -frounding-math) # CGAL!
  ENDIF()

  #list(SORT def_list)

  set_property( TARGET ${the_target} APPEND PROPERTY TGT_OPTIONS ${opt_list} )

endfunction()

#############################################################################################################

function(ork_std_target_opts_compiler the_target)

  ork_std_target_set_opts(${the_target})
  ork_std_target_set_defs(${the_target})
  ork_std_target_set_incdirs(${the_target})

  ################################################################################
  # standardized header search paths
  ################################################################################

  get_property( TGT_INCLUDE_PATHS TARGET ${the_target} PROPERTY TGT_INCLUDE_PATHS )
  target_include_directories(${the_target} PRIVATE ${TGT_INCLUDE_PATHS} )

  ################################################################################
  # standardized compile options
  ################################################################################

  get_property( TGT_OPTIONS TARGET ${the_target} PROPERTY TGT_OPTIONS )
  target_compile_options(${the_target} PRIVATE ${TGT_OPTIONS})

  ################################################################################
  # standardized definitions
  ################################################################################

  get_property( TGT_DEFINITIIONS TARGET ${the_target} PROPERTY TGT_DEFINITIIONS )
  target_compile_definitions(${the_target} PRIVATE ${TGT_DEFINITIIONS} )

endfunction()

#############################################################################################################

function(ork_lev2_target_opts_compiler the_target)
  ork_std_target_opts_compiler(${the_target})
  target_include_directories (${the_target} PRIVATE ${ORKROOT}/ork.core/inc )
  target_include_directories (${the_target} PRIVATE ${ORKROOT}/ork.lev2/inc )
  target_include_directories (${the_target} PRIVATE ${SRCD} )
  set_target_properties(${the_target} PROPERTIES LINKER_LANGUAGE CXX)
endfunction()

#############################################################################################################

function( ork_ecs_target_opts_compiler the_target)
  ork_lev2_target_opts_compiler(${the_target})
  target_include_directories (${the_target} PRIVATE ${ORKROOT}/ork.core/inc )
  target_include_directories (${the_target} PRIVATE ${ORKROOT}/ork.lev2/inc )
  target_include_directories (${the_target} PRIVATE ${ORKROOT}/ork.ecs/inc )
  target_include_directories (${the_target} PRIVATE ${ORKROOT}/ork.ecs/src )
  target_include_directories (${the_target} PRIVATE ${SRCD} )
  target_include_directories (${the_target} PRIVATE $ENV{OBT_STAGE}/include/luajit-2.1 )
endfunction()

#############################################################################################################

function(ork_std_target_set_libdirs the_target)

  set( private_libdir_list "" )
  list(APPEND private_libdir_list $ENV{OBT_STAGE}/lib )
  list(APPEND private_libdir_list $ENV{OBT_STAGE}/orkid/ork.tuio )
  list(APPEND private_libdir_list $ENV{OBT_STAGE}/orkid/ork.utpp )
  list(APPEND private_libdir_list $ENV{OBT_STAGE}/orkid/ork.core )
  list(APPEND private_libdir_list $ENV{OBT_STAGE}/orkid/ork.lev2 )
  list(APPEND private_libdir_list $ENV{OBT_STAGE}/orkid/ork.ecs )

  ################################################################################
  # IGL (its a beast, needs a cmake update)
  ################################################################################

  IF( "${ARCHITECTURE}" STREQUAL "x86_64" )

    list(APPEND CMAKE_MODULE_PATH "${LIBIGL_INCLUDE_DIR}/../cmake")
    option(LIBIGL_USE_STATIC_LIBRARY "Use libigl as static library" ON)
    #option(LIBIGL_WITH_ANTTWEAKBAR      "Use AntTweakBar"    OFF)
    option(LIBIGL_WITH_CGAL             "Use CGAL"           ON)
    option(LIBIGL_WITH_COMISO           "Use CoMiso"         ON)
    option(LIBIGL_WITH_CORK             "Use Cork"           ON)
    option(LIBIGL_WITH_EMBREE           "Use Embree"         ON)
    #option(LIBIGL_WITH_LIM              "Use LIM"            OFF)
    #option(LIBIGL_WITH_MATLAB           "Use Matlab"         OFF)
    #option(LIBIGL_WITH_MOSEK            "Use MOSEK"          OFF)
    #option(LIBIGL_WITH_OPENGL           "Use OpenGL"         ON)
    #option(LIBIGL_WITH_OPENGL_GLFW      "Use GLFW"           ON)
    #option(LIBIGL_WITH_PNG              "Use PNG"            OFF)
    #option(LIBIGL_WITH_PYTHON           "Use Python"         OFF)
    option(LIBIGL_WITH_TETGEN           "Use Tetgen"         ON)
    option(LIBIGL_WITH_TRIANGLE         "Use Triangle"       ON)
    #option(LIBIGL_WITH_VIEWER           "Use OpenGL viewer"  ON)
    #option(LIBIGL_WITH_XML              "Use XML"            OFF)
    find_package(LIBIGL REQUIRED)
    #include($ENV{OBT_BUILDS}/igl/cmake/libigl.cmake )

    list(APPEND private_libdir_list $ENV{OBT_BUILDS}/igl/.build)

  ENDIF()

  set_property( TARGET ${the_target} APPEND PROPERTY TGT_PRIVATE_LIBPATHS PRIVATE ${private_libdir_list} )
  set_property( TARGET ${the_target} APPEND PROPERTY TGT_PUBLIC_LIBPATHS PUBLIC $ENV{OBT_PYTHON_LIB_PATH}  )

endfunction()

#############################################################################################################

function(ork_std_target_opts_compiler_module the_target )
  ork_std_target_set_opts(${the_target})
  ork_std_target_set_defs(${the_target})
  ork_std_target_set_incdirs(${the_target})
  ork_std_target_set_libdirs(${the_target})
endfunction()

#############################################################################################################

function(ork_std_target_opts_linker the_target)
  ork_std_target_set_libdirs(${the_target})
  get_property( TGT_PRIVATE_LIBPATHS TARGET ${the_target} PROPERTY TGT_PRIVATE_LIBPATHS )
  get_property( TGT_PUBLIC_LIBPATHS TARGET ${the_target} PROPERTY TGT_PUBLIC_LIBPATHS )

  target_link_directories(${the_target} PRIVATE ${TGT_PRIVATE_LIBPATHS} )
  target_link_directories(${the_target} PUBLIC ${TGT_PUBLIC_LIBPATHS} )

  IF(${APPLE})
    target_link_directories(${the_target} PUBLIC /usr/local/lib )
      target_link_libraries(${the_target} LINK_PRIVATE m pthread)
      target_link_libraries(${the_target} LINK_PRIVATE
          "-framework AppKit"
          "-framework IOKit"
      )
      target_link_libraries(${the_target} LINK_PRIVATE objc boost_filesystem  boost_system boost_program_options )
  ELSEIF(${UNIX})
      target_link_libraries(${the_target} LINK_PRIVATE rt dl pthread boost_filesystem boost_system boost_program_options)
  ENDIF()
  target_link_libraries(${the_target} LINK_PRIVATE $ENV{OBT_PYTHON_DECOD_NAME} )
  #target_link_options(${TARGET} PRIVATE ${Python3_LINK_OPTIONS})
endfunction()

#############################################################################################################

function(ork_std_target_opts the_target)
  ork_std_target_opts_compiler(${the_target})
  ork_std_target_opts_linker(${the_target})
  IF(${APPLE})
  target_link_directories(${the_target} PRIVATE /usr/local/lib) # homebrew
  ENDIF()
endfunction()

#############################################################################################################
# ISPC compile option
#############################################################################################################

function(declare_ispc_source_object src obj dep)
  add_custom_command(OUTPUT ${obj}
                     MAIN_DEPENDENCY ${src}
                     COMMENT "ISPC-Compile ${src}"
                     COMMAND ispc -O3 --target=avx ${src} -g -o ${obj} --colored-output
                     DEPENDS ${dep})
endfunction()

#############################################################################################################
# ISPC convenience method
#  I really dislike cmake as a language...
#############################################################################################################

function(gen_ispc_object_list
         ISPC_GLOB_SPEC
         ISPC_SUBDIR
         ISPC_OUTPUT_OBJECT_LIST )
  set(ISPC_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/${ISPC_SUBDIR})
  #message(${ISPC_GLOB_SPEC})
  #message(${ISPC_SUBDIR})
  #message(${ISPC_OUTPUT_DIR})
  file(GLOB_RECURSE SRC_ISPC ${ISPC_GLOB_SPEC} )
  foreach(SRC_ITEM ${SRC_ISPC})
    file(RELATIVE_PATH SRC_ITEM_RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src ${SRC_ITEM} )
    get_filename_component(ispc_name_we ${SRC_ITEM_RELATIVE} NAME_WE)
    get_filename_component(ispc_dir ${SRC_ITEM_RELATIVE} DIRECTORY)
    set(OBJ_ITEM ${ISPC_OUTPUT_DIR}/${ispc_name_we}.o)
    #message(${SRC_ITEM})
    #message(${ispc_name_we})
    #message(${ispc_dir})
    #message(${OBJ_ITEM})
    declare_ispc_source_object(${SRC_ITEM} ${OBJ_ITEM} ${SRC_ITEM} )
    list(APPEND _INTERNAL_ISPC_OUTPUT_OBJECT_LIST ${OBJ_ITEM} )
  endforeach(SRC_ITEM)
  #message(${_INTERNAL_ISPC_OUTPUT_OBJECT_LIST})
  set(${ISPC_OUTPUT_OBJECT_LIST} ${_INTERNAL_ISPC_OUTPUT_OBJECT_LIST} PARENT_SCOPE)
endfunction()

#############################################################################################################
