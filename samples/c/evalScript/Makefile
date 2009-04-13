#
#	Makefile to build the evalScript sample
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

EXPORT_OBJECTS	:= 0
BUILD_NATIVE	:= 0

include 		.makedep

compileExtra: main$(BLD_EXE)

main$(BLD_EXE) : $(OBJECTS) $(BLD_EJS_MAKEDEP)
	@bld --exe main$(BLD_EXE) --shared --search "$(BLD_EJS_LIBPATHS)" --libs "ec $(BLD_EJS_LIBS)" $(OBJECTS)

cleanExtra:
	@rm -f main$(BLD_EXE)
