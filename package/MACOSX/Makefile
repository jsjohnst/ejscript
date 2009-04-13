#
#	Makefile to build a MAC package
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

include 	.makedep

PACKS		:= binary source dev
ROOT_DIR 	:= $(BLD_TOP)/.tmp

packageExtra:
	@priv  "export BLD_TOP=$$BLD_TOP TASK=Package LAUNCH=0 DRY_RUN=$(DRY_RUN) TRACE=$(TRACE) ROOT_DIR=$(ROOT_DIR); set -e ; \
			$(BLD_TOOLS_DIR)/makeInstall $(PACKS) ; $(BLD_TOOLS_DIR)/makePackage $(PACKS)"

packageOnly:
	@priv "export BLD_TOP=$$BLD_TOP TASK=Package LAUNCH=0 DRY_RUN=$(DRY_RUN) TRACE=$(TRACE) ROOT_DIR=$(ROOT_DIR); \
			$(BLD_TOOLS_DIR)/makePackage $(PACKS)"

cleanExtra:
	@rm -fr .pbldtmp rpm.out

clobberExtra:
	@rm -rf "$(ROOT_DIR)/*"
