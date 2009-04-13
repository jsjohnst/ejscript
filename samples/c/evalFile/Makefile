#
#	Makefile to build the evalFile sample
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

EXPORT_OBJECTS	:= 0
BUILD_NATIVE	:= 0

include 		.makedep

compileExtra: main$(BLD_EXE) test.mod

test.mod: Test.es
	$(call setlibpath) ; ejsc --out test.mod Test.es

main$(BLD_EXE) : $(OBJECTS) $(BLD_EJS_MAKEDEP)
	@bld --exe main$(BLD_EXE) --shared --search "$(BLD_EJS_LIBPATHS)" --libs "$(BLD_EJS_LIBS)" $(OBJECTS)

cleanExtra:
	@rm -f *.mod main$(BLD_EXE)
