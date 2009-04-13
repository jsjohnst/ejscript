#
#	Makefile for Appweb
#
#	Copyright (c) Embedthis Software LLC, 2003-2008. All Rights Reserved.
#
#	This is an all-in-one build where all the Appweb source files are catenated into as few files as possible. 
#	See http://hg.embedthis.com/appweb if you require the full source for the Appweb.
#

include 	.makedep

MAKE_IFLAGS		+= $(BLD_OPENSSL_IFLAGS) $(BLD_MATRIXSSL_IFLAGS)

compileExtra: $(BLD_LIB_DIR)/ejswebserver.conf $(BLD_LIB_DIR)/mime.types

$(BLD_LIB_DIR)/ejswebserver.conf: ejswebserver.conf
	cp ejswebserver.conf $(BLD_LIB_DIR)

$(BLD_LIB_DIR)/mime.types: mime.types
	cp mime.types $(BLD_LIB_DIR)

