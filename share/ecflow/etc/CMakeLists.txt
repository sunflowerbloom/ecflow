#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# the files we want to install
set (files
  ecflowview_menus.json
  ecflowview_attribute_conf.json
  ecflowview_gui.json
  ecflowview_gui_server.json
  ecflowview_highlighter.json
  ecflowview_icon_conf.json
  ecflowview_menu_conf.json
  ecflowview_network_conf.json
  ecflowview_notification_conf.json
  ecflowview_nstate_conf.json
  ecflowview_palette.json
  ecflowview_panels.json
  ecflowview_panel_conf.json
  ecflowview_query_conf.json
  ecflowview_sstate_conf.json
  ecflowview_server_conf.json
  ecflowview_view_conf.json
  ecflow_ui_test.def
  ecflow_ui_test_session_template.json
  ecflow_ui_test_server_scripts.tar.gz
  shortcuts.txt
  viewer.qss
)

# for each file, copy it into the build directory at build time
# and install it into the installation directory at install time (!)
foreach( f ${files} )
  configure_file(${f} ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
    
  install(
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/${f}
    DESTINATION
      share/ecflow/etc
    PERMISSIONS
      OWNER_WRITE
      OWNER_READ
      GROUP_READ
      WORLD_READ
  )
endforeach()
