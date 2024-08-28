# **********************************************************************
#
# Short description:
# Makefile template for DSD Private Implementation Library
# **********************************************************************
#
# Ericsson AB 2010 All rights reserved.
# The information in this document is the property of Ericsson.
# Except as specifically authorized in writing by Ericsson, the receiver of this
# document shall keep the information contained herein confidential and shall protect
# the same in whole or in part from disclosure and dissemination to third parties.
# Disclosure and disseminations to the receivers employees shall only be made
# on a strict need to know basis.
#
# **********************************************************************
#
# Rev        Date         Name      What
# -----      -------      --------  --------------------------
#            2012-10-25   eeritau   Created
#            2018-11-01   eanncze   Minor changes due to SwM2.0
# **********************************************************************
CNZ_NAME ?= EvHandlClient
CAA_NAME ?= EvHandlClientcmd_caa
CXC_NAME ?= EvHandlClient_cxc

# Basic paths
VOB_PATH = $(EXT_SUP_ROOT)/BSC
CNZ_PATH = $(VOB_PATH)/$(CNZ_NAME)
CAA_PATH = $(CNZ_PATH)/$(CAA_NAME)
CXC_PATH = $(CNZ_PATH)/$(CXC_NAME)

include $(COMMON_ROOT)/common.mk

DOXYGENFILECONFIG := $(COMMON_ROOT)/doxygen_common.conf

DOCDIR = $(CXC_PATH)/doc
DOXYGEN_OUTPUT=$(DOCDIR)/docgen
BLOCK_VERSION=1_0
BLOCK_SEARCH=./src ./inc_int
DEBUGDIR=$(OUTDIR)/.debug

CFLAGS += -std=c++0x

# libssh2 include files
#LIBSDIR += -L$(LIB_SSH2_SDK_LIB)
#LIBSDIR += -L"$(AP_SDK_LIB)"
#LIBSDIR += -L"$(3PP_SDK_PATH)/log4cplus/lib"

# here you can add own linker flags
#LDFLAGS += -s

## # here you can add own Include paths and/or other INCDIRludes
#CINCLUDES += -I"$(CAA_CMD_DIR)"

# libssh2 include files
#CINCLUDES += -I$(LIB_SSH2_SDK_INC)
#CINCLUDES += -I"$(AP_SDK_INC)"
#CINCLUDES += -I"$(3PP_SDK_PATH)"

# here you can add own libs
#LIBS += -lrt -lpthread -lacs_tra -llog4cplus -lacs_apgcc -lacs_aeh

## # here you can add own libs
#LIBS += -lssh2
LIBS += -lcap
LIBS += -lpthread 

.phony: all clean distclean

OUTDIR = ../EvHandlClient_cxc/bin

EVHANDLCLIENT_OBJ = $(OBJDIR)/evhandl_client.obj

EVHANDLCLIENT_APNAME = evhandlclient

EVHANDLCLIENT_APNAME_DBG = evhandlclient_dbg

EVHANDLCLIENT_APEXE = $(OUTDIR)/$(EVHANDLCLIENT_APNAME)

VPATH += $(SRCDIR) $(OUTDIR) $(INCDIR) $(OBJDIR) $(CAA_API_DIR)

.PHONY: all CFLAGS += $(GCOV_FLAGS)
all: $(OUTDIR)/$(EVHANDLCLIENT_APNAME)

$(OUTDIR)/$(EVHANDLCLIENT_APNAME): $(EVHANDLCLIENT_OBJ)
	$(NEW_LINE)
	$(SEPARATOR_STR)
	$(SILENT)$(ECHO) 'Creating Application: $(EVHANDLLIENT_APNAME)'
	$(SILENT)$(CC) $(CFLAGS) -o $(EVHANDLCLIENT_APEXE) $(EVHANDLCLIENT_OBJ) $(LDFLAGS) $(LIBSDIR) $(LIBS)
	$(call stripp,$(EVHANDLCLIENT_APNAME))
	$(SEPARATOR_STR)
	$(NEW_LINE)

.PHONY: clean
clean:
	$(RM) -r $(OBJDIR)/*.obj
	$(RM) -r $(OBJDIR)/*.d

.PHONY: distclean
distclean: clean
	$(RM) -r $(OUTDIR)/$(EVHANDLCLIENT_APNAME)

