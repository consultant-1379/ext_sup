# **********************************************************************
#
# Short description:
# Makefile template for XXX library
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
# -----      ----------   --------  --------------------------
# R1A        2013-01-08   eeritau   Created
# R2A        2018-11-01   eanncze   Changes due to SwM2.0 in APG43L 3.6
#                                                            
# **********************************************************************

CURDIR = $(shell pwd)
REPO_NAME = ext_sup
EXT_SUP_ROOT = $(shell echo $(CURDIR) | sed 's@'/$(REPO_NAME)'.*@'/$(REPO_NAME)'@g')
COMMON_ROOT = $(EXT_SUP_ROOT)/common

include $(COMMON_ROOT)/common.mk

CXC_NAME?=BSC_EVHANDLCLIENT
CXC_NR?=CXC1462066
CXC_VER?=R2C01
CXC_OLD_VER?=R1A01
BASE_SW_VER ?= 3.6.0-R1A
CXC_BLK_NAME ?= bsc_evhandlclient
CURDIR = $(shell pwd)
CXCPATH=$(CURDIR)
CXCDIR=$(CXCPATH)
export CXCDIR

#DOXYGEN_OUTPUT=$(DOCDIR)/docgen
BLOCK_NAME=$(CXC_NAME)
BLOCK_VERSION=$(CXC_NAME)_$(CXC_VER)
BLOCK_SEARCH=../EvHandlClientcmd_caa/src
# export DOXYGEN_OUTPUT
export BLOCK_NAME
export BLOCK_VERSION
export BLOCK_SEARCH

CAA_CMD_DIR = ../EvHandlClientcmd_caa/inc
export CAA_CMD_DIR

CAA_DIRS = \
	 ../EvHandlClientcmd_caa 

.PHONY: all
all: 
	@for i in $(CAA_DIRS) ; do \
		(echo "+++++++++++++++++" ; \
		echo "cd $$i" ; \
		cd $$i ; \
		$(MAKE) $(MFLAGS) COMMON_ROOT=$(COMMON_ROOT) EXT_SUP_ROOT=$(EXT_SUP_ROOT) all) ;\
		if [ $$? != 0 ] ; \
		then $(ECHO) "Error: please see the printout !!!";\
		exit $$?;\
		fi;\
		echo "+++++++++++++++++" ; \
	done

.PHONY: release
release: all rpm_package sdp_package esm_package #tar_package ap_sdk documentation metrics

.PHONY: nightbuild
nightbuild: distclean all rpm_package sdp_package esm_package copy_package #documentation metrics doc_package

.PHONY: documentation
documentation:
	$(call doc_creation)

.PHONY: clean
clean:
	@for i in $(CAA_DIRS) ; do \
		(echo "+++++++++++++++++" ; \
		 echo "cd $$i" ; \
		 cd $$i ; \
		 $(MAKE) $(MFLAGS) COMMON_ROOT=$(COMMON_ROOT) EXT_SUP_ROOT=$(EXT_SUP_ROOT) clean );\
		if [ $$? != 0 ] ; \
		then $(ECHO) "Error: please see the printout !!!";\
		exit $$?;\
		fi;\
		 echo "+++++++++++++++++" ; \
	done
	$(call FX_CLEAN_TMP)

.PHONY: distclean
distclean:
	@for i in $(CAA_DIRS) ; do \
		(echo "+++++++++++++++++" ; \
		echo "cd $$i" ; \
		cd $$i ; \
		$(MAKE) $(MFLAGS) COMMON_ROOT=$(COMMON_ROOT) EXT_SUP_ROOT=$(EXT_SUP_ROOT) distclean ) ;\
		if [ $$? != 0 ] ; \
		then $(ECHO) "Error: please see the printout !!!";\
		exit $$?;\
		fi;\
		echo "+++++++++++++++++" ; \
	done
	$(call FX_CLEAN_TMP)
.PHONY: rpm_package
rpm_package:
	$(call rpm_file_creation,evhandlclient.spec,$(CXC_NAME),$(CXC_NR),$(CXC_VER))


.PHONY: sdp_package
sdp_package:
	$(SILENT)$(ECHO) 'SDP packaging ...'
	$(call sdp_creation_US1,$(CXCPATH),$(BLOCK_NAME),$(CXC_VER),$(CXC_OLD_VER),$(CXC_NR))
	$(call up_creation,$(CXC_NAME),$(CXC_NR),$(CXC_VER),$(CXC_OLD_VER),$(CXCPATH),$(BASE_SW_VER))

.PHONY: esm_package
esm_package:
	$(call esm_pkg_creation,$(CXCPATH),$(CXC_BLK_NAME),$(CXC_VER),$(BASE_SW_VER),$(CXC_NR),$(CXC_NAME))

.PHONY: upload_package
upload_package:
	$(call upload_package,$(CXC_BLK_NAME),$(CXC_NR),$(CXC_NAME),$(CXC_VER))

.PHONY: copy_package
copy_package:
	$(call copy_file_to_integration,$(RPMDIR)/$(CXC_NAME)-$(CXC_NR)-$(CXC_VER).x86_64.rpm)
	$(call copy_file_to_integration,$(CXC_NAME)-$(CXC_NR)-$(CXC_VER)-documentation.tar)

.PHONY: doc_package
doc_package: documentation metrics
	$(SILENT)$(ECHO) 'DOC and Metrics packaging ...'
	$(call doc_tarfile_creation,$(CXC_NAME)-$(CXC_NR)-$(CXC_VER)-documentation.tar)

.PHONY: tar_package
tar_package:
	$(SILENT)$(ECHO) 'TAR packaging ...'
	$(SILENT)$(ECHO) '**********************************'
	$(SILENT)$(ECHO) '****** NOT YET IMPLEMENTED *******'
	$(SILENT)$(ECHO) '**********************************'

.PHONY: ap_sdk
ap_sdk:
	$(SILENT)$(ECHO) 'AP SDK updating ...'
	@for i in $(CAA_DIRS) ; do \
		(echo "+++++++++++++++++" ; \
		echo "cd $$i" ; \
		cd $$i ; \
		$(MAKE) $(MFLAGS) COMMON_ROOT=$(COMMON_ROOT) EXT_SUP_ROOT=$(EXT_SUP_ROOT) ap_sdk ) ;\
		if [ $$? != 0 ] ; \
		then $(ECHO) "Error: please see the printout !!!";\
		exit $$?;\
		fi;\
		echo "+++++++++++++++++" ; \
	done

.PHONY: undo_ap_sdk
undo_ap_sdk:
	$(SILENT)$(ECHO) 'UNDO AP SDK packaging ...'
	$(SILENT)$(ECHO) '**********************************'
	$(SILENT)$(ECHO) '****** NOT YET IMPLEMENTED *******'
	$(SILENT)$(ECHO) '**********************************'


