#
#	Makefile for the ejs projects
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

include     .makedep

MPR_HEADERS = $(shell find ../src/include -name 'mpr*.h')
EJS_HEADERS = $(shell find ../src/include -name 'ejs*.h')
EC_HEADERS  = $(shell find ../src/include -name 'ec*.h')
MPR_SOURCES = $(shell find ../src/mpr -name '*.c' | egrep -v 'http.c|makerom.c')
EJS_SOURCES = $(shell find ../src/db ../src/types ../src/vm ../src/web -name '*.c' | \
	egrep -v 'ejsweb.c|ejssql.c|ejsAppweb.c|ejsCgi.c')
EC_SOURCES  = $(shell find ../src/compiler -name '*.c')

projects: ejs.vcproj ejsc.vcproj ejsvm.vcproj ejsmod.vcproj ejssql.vcproj ejsweb.vcproj http.vcproj

ejs.vcproj: dummy
	@makeVsProj --project ejs.vcproj --exe ejs.exe --search "$(BLD_EJS_LIBPATHS)" \
		--libs "ws2_32.lib" $(MPR_HEADERS) $(EJS_HEADERS) $(EJS_SOURCES) $(EC_SOURCES) $(MPR_SOURCES) ../src/cmd/ejs.c

ejsc.vcproj: dummy
	@makeVsProj --project ejsc.vcproj --exe ejsc.exe --search "$(BLD_EJS_LIBPATHS)" \
		--libs "ws2_32.lib" $(MPR_HEADERS) $(EJS_HEADERS) $(EC_HEADERS) $(EJS_SOURCES) $(EC_SOURCES) $(MPR_SOURCES) \
		../src/cmd/ejsc.c

ejsvm.vcproj: dummy
	@makeVsProj --project ejsvm.vcproj --exe ejsvm.exe --search "$(BLD_EJS_LIBPATHS)" \
		--libs "ws2_32.lib" $(MPR_HEADERS) $(EJS_HEADERS) $(EJS_SOURCES) $(MPR_SOURCES) ../src/cmd/ejsvm.c

ejsmod.vcproj: dummy
	@makeVsProj --project ejsmod.vcproj --exe ejsmod.exe --search "$(BLD_EJS_LIBPATHS)" \
		--libs "ws2_32.lib" $(MPR_HEADERS) $(EJS_HEADERS) $(EJS_SOURCES) $(MPR_SOURCES) \
        ../src/ejsmod/ejsmod.c ../src/ejsmod/doc.c ../src/ejsmod/docFiles.c ../src/ejsmod/listing.c ../src/ejsmod/slotGen.c

ejssql.vcproj: dummy
	@makeVsProj --project ejssql.vcproj --exe ejssql.exe --search "$(BLD_MPR_LIBPATHS)" \
		--libs "ws2_32.lib" $(MPR_HEADERS) $(MPR_SOURCES) ../src/db/sqlite/ejssql.c ../src/db/sqlite/sqlite3.c \
        ../src/db/sqlite/sqliteMem.c

ejsweb.vcproj: dummy
	@makeVsProj --project ejsweb.vcproj --exe ejsweb.exe --search "$(BLD_MPR_LIBPATHS)" \
		--libs "ws2_32.lib" $(MPR_HEADERS) $(MPR_SOURCES) ../src/cmd/ejsweb.c

http.vcproj: dummy
	@makeVsProj --project http.vcproj --exe http.exe --search "$(BLD_MPR_LIBPATHS)" \
		--libs "ws2_32.lib" $(MPR_HEADERS) $(MPR_SOURCES) ../src/mpr/http.c
