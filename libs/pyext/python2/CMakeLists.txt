#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

ecbuild_add_library(
  TARGET
    ecflow2
  NOINSTALL
  TYPE MODULE
  SOURCES
    ${srcs}
  PRIVATE_INCLUDES
    ../src
  PUBLIC_LIBS
    ecflow_all
    libsimulator
    Python2::Python
    Boost::${ECFLOW_BOOST_PYTHON_COMPONENT}
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
  CXXFLAGS
    $<$<CXX_COMPILER_ID:Clang>:-Wno-macro-redefined>
)

target_clangformat(ecflow2)

#
# Override default behaviour that add RPATHS during install
# The only thing that seem to work is set INSTALL_RPATH to ""
# Using SKIP_BUILD_RPATH,BUILD_WITH_INSTALL_RPATH,INSTALL_RPATH_USE_LINK_PATH
# had no effect
#         
# by default cmake add prefix 'lib', we don't want this hence disable
#
# To avoid duplicate target names we chose to name the targets: ecflow2/ecflow3.
# however test and user code  depend on name 'ecflow', hence we rename the output to 'cflow'
set_target_properties(ecflow3
  PROPERTIES
    OUTPUT_NAME "ecflow"
    PREFIX ""
    INSTALL_RPATH ""
)

# =====================================================================
# tests

foreach( test ${u_tests} )
  ecbuild_add_test(
    TARGET
      py2_${test}
    LABELS
      python nightly
    TYPE PYTHON
    ARGS
      ${CMAKE_CURRENT_SOURCE_DIR}/../test/${test}.py
    ENVIRONMENT
      "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}"
    TEST_DEPENDS
      u_base
  )
endforeach()


if ( ENABLE_ALL_TESTS AND ENABLE_SERVER)

  foreach( test ${s_tests} )
    ecbuild_add_test(
      TARGET
        py2_${test}
      LABELS
        python nightly
      TYPE PYTHON
      ARGS
        ${CMAKE_CURRENT_SOURCE_DIR}/../test/${test}.py
      ENVIRONMENT
        "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}"
      TEST_DEPENDS
        u_base
    )
  endforeach()
  
  set_property(TEST py2_s_TestClientApi      APPEND PROPERTY DEPENDS s_test)
  set_property(TEST py2_s_TestPythonChildApi APPEND PROPERTY DEPENDS py_s_TestClientApi)
endif()


# ==========================================================================
# install
#    -DCMAKE_PYTHON_INSTALL_TYPE = [ local | setup | not defined ]
#
#    local | not defined : this will install to:
#                          $INSTALL_PREFIX/$release.$major.$minor/lib/python2.7/site-packages/ecflow/
#    setup               : experimental only,python way of installing
#
#    -DCMAKE_PYTHON_INSTALL_PREFIX should *only* used when using python setup.py (CMAKE_PYTHON_INSTALL_TYPE=setup)
#    *AND* for testing python install to local directory
#
# Note:  To install only the python module
#       cd  buildir
#       cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DCOMPONENT=python -P cmake_install.cmake -- make install
# ==========================================================================

if( CMAKE_PYTHON_INSTALL_TYPE MATCHES  "local"  OR NOT DEFINED CMAKE_PYTHON_INSTALL_TYPE )

  message(STATUS "python install *LOCAL* +++ lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages/ecflow +++++++")
  if( NOT INSTALL_PYTHON2_DIR )
    set(PYTHON_SITE "lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages" )
  else()
    # Resolve ${VAR} in the value provided
    string(CONFIGURE "${INSTALL_PYTHON2_DIR}" PYTHON_SITE)
  endif()
  set(PYTHON_DEST "${PYTHON_SITE}/ecflow" )

  install(
    TARGETS
      ecflow2
    DESTINATION
      ${PYTHON_DEST}
    RENAME
      ecflow.so
    COMPONENT
      python
  )
  install(
    FILES
      ../ecflow/__init__.py
      ../samples/api/ecf.py
      ../samples/api/sms2ecf.py
    DESTINATION
      ${PYTHON_DEST}
    COMPONENT
      python
  )

else()

  message( STATUS "python found,  CMAKE_PYTHON_INSTALL_TYPE=${CMAKE_PYTHON_INSTALL_TYPE}")

  # -------------------------------------------------------------------------------------
  # Install using setup.py
  # See: http://bloerg.net/2012/11/10/cmake-and-distutils.html
  # -------------------------------------------------------------------------------------
  message(STATUS "python install using *setup.py* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
  message(STATUS "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
  message(STATUS "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}")
  message(STATUS "CMAKE_PYTHON_INSTALL_PREFIX : ${CMAKE_PYTHON_INSTALL_PREFIX}" )

  set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/../setup.py.in")
  set(SETUP_PY    "${CMAKE_CURRENT_SOURCE_DIR}/../setup.py")
  set(DEPS        "${CMAKE_CURRENT_SOURCE_DIR}/../ecflow/__init__.py")
  set(OUTPUT      "${CMAKE_CURRENT_SOURCE_DIR}/timestamp")

  configure_file(${SETUP_PY_IN} ${SETUP_PY} )

  add_custom_command(
    OUTPUT ${OUTPUT}
    COMMAND ${PYTHON} ${SETUP_PY} build
    COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
    DEPENDS ${DEPS}
  )
  add_custom_target(target ALL DEPENDS ${OUTPUT})

  install(CODE "execute_process(COMMAND ${PYTHON} ${SETUP_PY} build_ext)")

  if( DEFINED CMAKE_PYTHON_INSTALL_PREFIX )
    message(STATUS "custom/*test* python install prefix defined CMAKE_PYTHON_INSTALL_PREFIX=${CMAKE_PYTHON_INSTALL_PREFIX}")
    install(CODE "execute_process(COMMAND ${PYTHON} ${SETUP_PY} install -f --prefix=${CMAKE_PYTHON_INSTALL_PREFIX})")
  else()
    install(CODE "execute_process(COMMAND ${PYTHON} ${SETUP_PY} install)")
  endif()
endif()
