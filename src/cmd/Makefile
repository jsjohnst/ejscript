#
#	Makefile for the Ejscript command line programs: ejsc, ejs, ejsvm
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

include 		.makedep

TARGETS			+= $(BLD_BIN_DIR)/ejsvm$(BLD_EXE) \
				   $(BLD_BIN_DIR)/ejsc$(BLD_EXE) \
				   $(BLD_BIN_DIR)/ejs$(BLD_EXE) 
ifeq ($(BLD_FEATURE_EJS_WEB),1)
	TARGETS		+= $(BLD_BIN_DIR)/ejsweb$(BLD_EXE)
endif

compileExtra: 	$(TARGETS) 

#
#	Make the compiler
#
$(BLD_BIN_DIR)/ejsc$(BLD_EXE): $(OBJECTS) $(BLD_LIB_DIR)/libejs$(BLD_LIB) $(BLD_LIB_DIR)/libec$(BLD_LIB) \
		$(BLD_LIB_DIR)/libmpr$(BLD_LIB)
	@bld --exe $(BLD_BIN_DIR)/ejsc$(BLD_EXE) --search "$(BLD_EJS_LIBPATHS)" --libs "ec $(BLD_EJS_LIBS)" \
		"$(BLD_OBJ_DIR)/ejsc$(BLD_OBJ)"

#
#	Make the C vm interpreter
#
$(BLD_BIN_DIR)/ejsvm$(BLD_EXE): $(OBJECTS) $(BLD_LIB_DIR)/libejs$(BLD_LIB) $(BLD_LIB_DIR)/libmpr$(BLD_LIB)
	@bld --exe $(BLD_BIN_DIR)/ejsvm$(BLD_EXE) --search "$(BLD_EJS_LIBPATHS)" --libs "$(BLD_EJS_LIBS)" \
		"$(BLD_OBJ_DIR)/ejsvm$(BLD_OBJ)"

#
#	Make the Ejscript shell
#
$(BLD_BIN_DIR)/ejs$(BLD_EXE): $(OBJECTS) $(BLD_LIB_DIR)/libejs$(BLD_LIB) $(BLD_LIB_DIR)/libec$(BLD_LIB) \
		$(BLD_LIB_DIR)/libmpr$(BLD_LIB)
	@bld --exe $(BLD_BIN_DIR)/ejs$(BLD_EXE) --search "$(BLD_EJS_LIBPATHS)" --libs "ec $(BLD_EJS_LIBS)" \
		"$(BLD_OBJ_DIR)/ejs$(BLD_OBJ)"

#
#	Make the ejsweb framework generator
#
$(BLD_BIN_DIR)/ejsweb$(BLD_EXE): $(OBJECTS) $(BLD_LIB_DIR)/libmpr$(BLD_LIB)
	@bld --exe $(BLD_BIN_DIR)/ejsweb$(BLD_EXE) --search "$(BLD_EJS_LIBPATHS)" --libs "ec $(BLD_EJS_LIBS)" \
		"$(BLD_OBJ_DIR)/ejsweb$(BLD_OBJ)"
