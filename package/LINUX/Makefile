#
#	Makefile to build a linux package
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

include 	.makedep

PACKS		:= binary source dev
ROOT_DIR 	:= $(BLD_TOP)/.tmp

packageExtra:
	fakePriv "export BLD_TOP=$(BLD_TOP) TASK=Package LAUNCH=0 DRY_RUN=$(DRY_RUN) TRACE=$(TRACE) \
		ROOT_DIR=$(ROOT_DIR); $(BLD_TOOLS_DIR)/makeInstall $(PACKS) ; \
		$(BLD_TOOLS_DIR)/makePackage $(PACKS)"

packageOnly:
	@fakePriv "export BLD_TOP=$$BLD_TOP TASK=Package LAUNCH=0 DRY_RUN=$(DRY_RUN) TRACE=$(TRACE) \
		ROOT_DIR=$(ROOT_DIR); $(BLD_TOOLS_DIR)/makePackage $(PACKS)"

cleanExtra:
	@rm -fr .pbldtmp rpm.out

clobberExtra:
	@rm -fr $(ROOT_DIR)/*
