#
#	Makefile for bin
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#
 
include 		.makedep

dependExtra:
	@[ "$(BLD_CROSS)" = 1 ] && mkdir -p $(BLD_HOST_SYSTEM) ; true

cleanExtra:
	@rm -f ec* ejs* makerom* http* ./$(BLD_HOST_SYSTEM)/* *.lib *.dll *.so appweb*
