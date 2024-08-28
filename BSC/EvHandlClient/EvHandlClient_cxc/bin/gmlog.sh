#!/bin/bash
##
# ------------------------------------------------------------------------
#     Copyright (C) 2012 Ericsson AB. All rights reserved.
# ------------------------------------------------------------------------
##
# Name:
#       gmlog.sh
# Description:
#       A script to wrap the invocation of evhandlclient from the COM CLI.
# Note:
#	None.
##
# Usage:
#	None.
##
# Output:
#       None.
##
# Changelog:
#	First version.
##

/usr/bin/sudo /opt/ap/ext/evhandlclient/bin/evhandlclient gmlog "$@"

exit $?
