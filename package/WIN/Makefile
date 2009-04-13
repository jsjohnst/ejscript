#
#	Makefile for the Windows release
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

include 	.makedep 

PACKS		:= binary source dev
ROOT_DIR 	:= $(BLD_TOP)/.tmp

ifeq	($(BLD_WIN_LIKE),1)
compileExtra: $(BLD_BIN_DIR)/removeFiles$(BLD_EXE)
endif

$(BLD_BIN_DIR)/removeFiles$(BLD_EXE): $(BLD_OBJ_DIR)/removeFiles$(BLD_OBJ)
	@bld --graphical --exe $(BLD_BIN_DIR)/removeFiles$(BLD_EXE) removeFiles$(BLD_OBJ)

packageExtra:
	priv "export BLD_TOP=$$BLD_TOP TASK=Package LAUNCH=0 DRY_RUN=$(DRY_RUN) TRACE=$(TRACE) ROOT_DIR=$(ROOT_DIR); set -e ; \
		$(BLD_TOOLS_DIR)/makeInstall $(PACKS) ; $(BLD_TOOLS_DIR)/makePackage $(PACKS)"

packageOnly:
	@priv "export BLD_TOP=$$BLD_TOP TASK=Package LAUNCH=0 DRY_RUN=$(DRY_RUN) TRACE=$(TRACE) ROOT_DIR=$(ROOT_DIR); \
			$(BLD_TOOLS_DIR)/makePackage $(PACKS)"

cleanExtra:
	@rm -fr pkgtmp input.iss .pbldtmp bin.iss make.out Output install.log

clobberExtra:
	rm -fr $(ROOT_DIR)/
