
.. _alter_cli:

alter
/////

::

   
   alter
   -----
   
   Alter the node according to the options.
   To add/delete/change server variables use '/' for the path.
   arg1 =   [ delete | change | add | set_flag | clear_flag | sort ]
            one option must be specified
   arg2 = For delete:
            [ variable | time | today | date  | day | cron | event | meter | late | generic | queue |
              label | trigger | complete | repeat | limit | inlimit | limit_path | zombie ]
          For change:
            [ variable | clock_type | clock_gain | clock_date | clock_sync  | event | meter | label |
              trigger  | complete   | repeat     | limit_max  | limit_value | defstatus | late | time | today ]
            *NOTE* If the clock is changed, then the suite will need to be re-queued in order for
            the change to take effect fully.
          For add:
            [ variable | time | today | date | day | zombie | late | limit | inlimit | label ]
          For set_flag and clear_flag:
            [ force_aborted | user_edit | task_aborted | edit_failed | ecfcmd_failed 
              statuscmd_failed | killcmd_failed | no_script | killed | status | late | message | 
              complete | queue_limit | task_waiting | locked | zombie | archived | restored |
              threshold | log_error | checkpt_error]
          For sort:
            [ event | meter | label | variable| limit | all ]
   arg3 = name/value
          when changing, attributes like variable,meter,event,label,limits,late
          we expect arguments to be quoted. For sort this argument can be called 'recursive'
   arg4 = new_value
          specifies the new value only used for 'change'/'add'
          values with spaces must be quoted
   arg5 = paths : At lease one node path required.The paths must start with a leading '/' character
   
   
   Usage:
   
      ecflow_client --alter=add variable GLOBAL "value" /           # add server variable
      ecflow_client --alter=add variable FRED "value" /path/to/node # add node variable
      ecflow_client --alter=add time "+00:20" /path/to/node
      ecflow_client --alter=add date "01.*.*" /path/to/node
      ecflow_client --alter=add day "sunday"  /path/to/node
      ecflow_client --alter=add label name "label_value" /path/to/node
      ecflow_client --alter=add late "-s 00:01 -a 14:30 -c +00:01" /path/to/node
      ecflow_client --alter=add limit mars "100" /path/to/node
      ecflow_client --alter=add inlimit /path/to/node/withlimit:limit_name "10" /path/to/node
      # zombie attributes have the following structure:
        `zombie_type`:(`client_side_action` | `server_side_action`):`child`:`zombie_life_time`
         zombie_type        = "user" | "ecf" | "path" | "ecf_pid" | "ecf_passwd" | "ecf_pid_passwd"
         client_side_action = "fob" | "fail" | "block"
         server_side_action = "adopt" | "delete" | "kill"
         child              = "init" | "event" | "meter" | "label" | "wait" | "abort" | "complete" | "queue"
         zombie_life_time   = unsigned integer default: user(300), ecf(3600), path(900)  minimum is 60
      ecflow_client --alter=add zombie "ecf:fail::" /path/to/node     # ask system zombies to fail
      ecflow_client --alter=add zombie "user:fail::" /path/to/node    # ask user generated zombies to fail
      ecflow_client --alter=add zombie "path:fail::" /path/to/node    # ask path zombies to fail
   
      ecflow_client --alter=delete variable FRED /path/to/node    # delete variable FRED
      ecflow_client --alter=delete variable      /path/to/node    # delete *ALL* variables on the specified node
   
   
   The client reads in the following environment variables. These are read by user and child command
   
   |----------|----------|------------|-------------------------------------------------------------------|
   | Name     |  Type    | Required   | Description                                                       |
   |----------|----------|------------|-------------------------------------------------------------------|
   | ECF_HOST | <string> | Mandatory* | The host name of the main server. defaults to 'localhost'         |
   | ECF_PORT |  <int>   | Mandatory* | The TCP/IP port to call on the server. Must be unique to a server |
   | ECF_SSL  |  <any>   | Optional*  | Enable encrypted comms with SSL enabled server.                   |
   |----------|----------|------------|-------------------------------------------------------------------|
   
   * The host and port must be specified in order for the client to communicate with the server, this can 
     be done by setting ECF_HOST, ECF_PORT or by specifying --host=<host> --port=<int> on the command line
   