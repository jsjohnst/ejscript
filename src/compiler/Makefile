#
#	Makefile for the Ejscript Compiler library
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

include 		.makedep

TARGETS			+= $(BLD_LIB_DIR)/libec$(BLD_LIB)

compileExtra: 	$(TARGETS)

$(BLD_LIB_DIR)/libec$(BLD_LIB): $(OBJECTS)
	@bld --library $(BLD_LIB_DIR)/libec --search "$(BLD_EJS_LIBPATHS) $(BLD_EJS_WITHPATHS)" \
		--libs "$(BLD_EJS_LIBS) $(BLD_EJS_WITHLIBS)" $(OBJECTS)
