#
#	Makefile for the MPR - Michael's Portable Runtime
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#
#	This is an all-in-one build where all the MPR source files are catenated into as few files as possible. 
#	See http://hg.embedthis.com/mpr if you require the full source for the MPR.
#

include 			.makedep

TARGETS				+= $(BLD_LIB_DIR)/libmpr$(BLD_LIB)
MPR_OBJECTS			+= mprLib
IMPORTS 			+= $(BLD_OPENSSL_IMPORTS) $(BLD_MATRIXSSL_IMPORTS) $(BLD_REGEXP_IMPORTS) $(BLD_MPR_IMPORTS)
IMPORTED			+= $(patsubst %,$(BLD_LIB_DIR)/%, $(notdir $(IMPORTS)))
MAKE_IFLAGS			+= $(BLD_OPENSSL_IFLAGS) $(BLD_MATRIXSSL_IFLAGS) $(BLD_REGEXP_IFLAGS) $(BLD_MPR_IFLAGS)

ifeq ($(BLD_FEATURE_REGEXP),1)
	TARGETS			+= $(BLD_LIB_DIR)/libpcre$(BLD_LIB)
endif

ifeq ($(BLD_FEATURE_SSL),1)
	TARGETS			+= $(BLD_LIB_DIR)/libmprssl$(BLD_LIB)
	SSL_OBJECTS		+= mprSsl
endif

ifeq ($(BUILDING_CROSS),0)
	TARGETS			+= $(BLD_BIN_DIR)/makerom$(BLD_EXE)
endif

ifeq ($(BLD_FEATURE_HTTP_CLIENT),1)
	TARGETS			+= $(BLD_BIN_DIR)/http$(BLD_EXE)
endif

compileTargets: imports $(TARGETS)

#
#	MPR library
#
$(BLD_LIB_DIR)/libmpr$(BLD_LIB): $(OBJECTS)
	@bld --library $(BLD_LIB_DIR)/libmpr $(MPR_OBJECTS)

#
#	Regular Expression (pcre) library
#
$(BLD_LIB_DIR)/libpcre$(BLD_LIB): $(OBJECTS)
	@bld --library $(BLD_LIB_DIR)/libpcre mprPcre

#
#	SSL library
#
$(BLD_LIB_DIR)/libmprssl$(BLD_LIB): $(OBJECTS)
	@bld --library $(BLD_LIB_DIR)/libmprssl --search "$(BLD_SSL_WITHLIBPATHS)" --libs "mpr $(BLD_SSL_WITHLIBS)" \
		$(SSL_OBJECTS)

#
#	makerom utility
#
$(BLD_BIN_DIR)/makerom$(BLD_EXE): $(OBJECTS) $(BLD_LIB_DIR)/libmpr$(BLD_LIB)
	@bld --exe $(BLD_BIN_DIR)/makerom$(BLD_EXE) --search "$(BLD_SSL_LIBPATHS)" --libs "$(BLD_MPR_LIBS) \
		$(BLD_SSL_LIBS)" makerom

#
#	http utility
#
$(BLD_BIN_DIR)/http$(BLD_EXE): $(OBJECTS) $(BLD_LIB_DIR)/libmpr$(BLD_LIB)
	@bld --exe $(BLD_BIN_DIR)/http$(BLD_EXE) --search "$(BLD_SSL_LIBPATHS)" --libs "$(BLD_MPR_LIBS) $(BLD_SSL_LIBS)" http

#
#	Import required libraries
#
imports: $(IMPORTED)

$(IMPORTED): $(IMPORTS)
	getlib $^

cleanExtra:
	@rm -f $(IMPORTED)
