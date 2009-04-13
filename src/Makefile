#
#	Makefile for the Ejscript source code.
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#
#	Building strategy is:
#	1. Build the core types (PRE_DIRS)
#	2. Build the core libraries (PRE_TARGETS)
#	3. Build the compiler and tools (MID_DIRS)
#	4. Generate the slot files (MID_DIRS - es)
#	5. See if ejs.slots.h changed, if so, clean and redo from step 1
#	6. Build remaining commands: ejssql, ejscgi, appweb and modules (TARGETS)
#

include 		.makedep


PRE_DIRS		+= include
ifeq ($(BLD_MPR_BUILTIN),1)
	PRE_DIRS	+= mpr
endif
PRE_DIRS		+= types regex vm db
ifeq ($(BLD_FEATURE_EJS_WEB),1)
	PRE_DIRS	+= web 
endif

MID_DIRS		+= compiler cmd ejsmod es
ifeq ($(BLD_FEATURE_APPWEB),1)
	POST_DIRS	+= appweb
endif
POST_DIRS		+= test

#
#	Targets. This builds the following targets:
#		Libraries:   ejs, sqlite3, ejsdb, ejsweb
#		Executables: ejssql, ejscgi
#		Modules:     mod_appweb_ejs, mod_ejs (Appweb and Apache ejs modules)
#
PRE_TARGETS			+= $(BLD_LIB_DIR)/libejs$(BLD_LIB)
ifeq ($(BLD_FEATURE_SQLITE),1)
	PRE_TARGETS		+= $(BLD_LIB_DIR)/libsqlite3$(BLD_LIB) 
	ifneq ($(BUILDING_CROSS),1)
		TARGETS		+= $(BLD_BIN_DIR)/ejssql$(BLD_EXE) 
	endif
endif
ifeq ($(BLD_FEATURE_EJS_DB),1)
	ifeq ($(BLD_FEATURE_STATIC),0)
		PRE_TARGETS	+= $(BLD_LIB_DIR)/libejsdb$(BLD_LIB)
	endif
endif
ifeq ($(BLD_FEATURE_EJS_WEB),1)
	ifeq ($(BLD_FEATURE_STATIC),0)
		PRE_TARGETS	+= $(BLD_LIB_DIR)/libejsweb$(BLD_LIB)
	endif
	TARGETS			+= $(BLD_BIN_DIR)/ejscgi$(BLD_EXE)
	ifeq ($(BLD_FEATURE_APPWEB),1)
		TARGETS		+= $(BLD_BIN_DIR)/ejswebserver$(BLD_EXE)
		ifneq	($(BLD_APPWEB_DIR),)
			TARGETS	+= $(BLD_BIN_DIR)/mod_appweb_ejs$(BLD_SHOBJ)
		endif
	endif
	## ifeq ($(BLD_FEATURE_APACHE),1)
	##	TARGETS		+= mod_ejs.la
	##endif
endif

#
#	If static building: include objects from db and web in the ejs library
#
ifeq ($(BLD_FEATURE_STATIC),1)
	ifeq ($(BLD_FEATURE_EJS_DB),1)
		EJS_SOURCES	+= $(wildcard db/*.c)
	endif
	ifeq ($(BLD_FEATURE_EJS_WEB),1)
		EJS_SOURCES	+= $(shell find web -name '*.c' | egrep -v '/gateways/|/proto/')
	endif
endif
EJS_SOURCES		+= $(shell find types vm -name '*.c') 
EJS_OBJECTS		+= $(patsubst %.c,$(BLD_OBJ_DIR)/%$(BLD_OBJ),$(notdir $(EJS_SOURCES)))
WEB_SOURCES		+= $(shell find web -name '*.c' | egrep -v '/gateways/|/proto/') 
WEB_OBJECTS		+= $(patsubst %.c,$(BLD_OBJ_DIR)/%$(BLD_OBJ),$(notdir $(WEB_SOURCES)))

compileFirst:
	@cp include/ejs.slots.h .ejs.slots.h

compileExtra: $(PRE_TARGETS)
	@+T=compile ; D="$(MID_DIRS)" ; $(DO_RECURSE) 
#
#	Build again if the slots change.
#
	@if ! diff include/ejs.slots.h .ejs.slots.h >/dev/null ; then \
		echo -e "\n#\n# Core slots may have changed. Rebuilding Compiler with current slot definitions.\n#\n" ; \
		rm -f $(BLD_LIB_DIR)/libejs$(BLD_LIB) ; \
		T=compile ; D="$(PRE_DIRS)" ; $(DO_RECURSE) ; \
		make $(BLD_LIB_DIR)/libejs$(BLD_LIB) ; \
		T=compile ; D="$(MID_DIRS)" ; $(DO_RECURSE) ; \
	fi
#
#	Now that we have a compiler, we can build any other modules
#
	@echo "# $(MAKE) -C \"./es\" modules"
	@make -C ./es modules

#
#	Build the all-in-one Ejscript distribution
#
ifneq ($(BUILDING_CROSS),1)
compileFinal: $(TARGETS)
	@echo "# $(MAKE) -C \"./all\" -f Makefile.all all-in-one"
	@make -C ./all -f Makefile.all all-in-one
endif

#
#	libejs - Core Ejscript language library
#
$(BLD_LIB_DIR)/libejs$(BLD_LIB): $(EJS_OBJECTS) $(BLD_LIB_DIR)/libmpr$(BLD_LIB)
	@bld --library $(BLD_LIB_DIR)/libejs --search "$(BLD_EJS_LIBPATHS)" --libs "$(BLD_EJS_WITHLIBS)" $(EJS_OBJECTS)

#
#	libejsweb - Ejscript web framework (only if shared)
#
$(BLD_LIB_DIR)/libejsweb$(BLD_LIB): $(WEB_OBJECTS)
	@bld --library $(BLD_LIB_DIR)/libejsweb$(BLD_LIB) --search "$(BLD_EJS_LIBPATHS)" --libs "ejs $(BLD_MPR_LIBS)" \
		$(WEB_OBJECTS)
ifeq ($(BLD_FEATURE_STATIC),0)
#
#	Used by the ejs loader when loading ejs.web.mod ejs.web.so
#
	cp $(BLD_LIB_DIR)/libejsweb$(BLD_LIB) $(BLD_MOD_DIR)/ejs.web$(BLD_LIB)
endif

#
#	libsqlite3 - Sqlite3 library
#
$(BLD_LIB_DIR)/libsqlite3$(BLD_LIB): $(BLD_OBJ_DIR)/sqlite3$(BLD_OBJ) $(BLD_OBJ_DIR)/sqliteMem$(BLD_OBJ)
	@bld --library $(BLD_LIB_DIR)/libsqlite3 --search "$(BLD_MPR_LIBPATHS)" --libs "$(BLD_MPR_LIBS)" sqlite3 sqliteMem

#
#	libejsdb - Ejscript database module (only if shared)
#
$(BLD_LIB_DIR)/libejsdb$(BLD_LIB): $(BLD_OBJ_DIR)/ejsDb$(BLD_OBJ) 
	@bld --library $(BLD_LIB_DIR)/libejsdb$(BLD_LIB) --search "$(BLD_MPR_LIBPATHS) $(BLD_SQLITE_LIBPATHS)" \
		--libs "ejs $(BLD_MPR_LIBS) $(BLD_SQLITE_LIBS)" ejsDb
ifeq ($(BLD_FEATURE_STATIC),0)
	cp $(BLD_LIB_DIR)/libejsdb$(BLD_LIB) $(BLD_MOD_DIR)/ejs.db$(BLD_LIB)
endif


#
#	ejssql - Sqlite command program
#
$(BLD_BIN_DIR)/ejssql$(BLD_EXE): $(BLD_LIB_DIR)/libsqlite3$(BLD_LIB)
	@bld --exe $(BLD_BIN_DIR)/ejssql$(BLD_EXE) --search "$(BLD_MPR_LIBPATHS)" --libs "$(BLD_MPR_LIBS)" \
		ejssql sqlite3 sqliteMem

#
#	mod_appweb_ejs - Appweb ejs module and handler
#
$(BLD_BIN_DIR)/mod_appweb_ejs$(BLD_SHOBJ): $(BLD_OBJ_DIR)/ejsAppweb$(BLD_OBJ) $(BLD_LIB_DIR)/libejs$(BLD_LIB)
	@bld --shared --library $(BLD_MOD_DIR)/mod_appweb_ejs --libs "$(BLD_MPR_LIBS)" \
		$(BLD_OBJ_DIR)/ejsAppweb$(BLD_OBJ)

#
#	ejscgi - CGI gateway program
#
$(BLD_BIN_DIR)/ejscgi$(BLD_EXE): $(BLD_OBJ_DIR)/ejsCgi$(BLD_OBJ) $(BLD_LIB_DIR)/libejs$(BLD_LIB)
	@bld --shared --exe $(BLD_BIN_DIR)/ejscgi$(BLD_EXE) --search "$(BLD_EJS_LIBPATHS)" --libs "$(BLD_EJS_LIBS)" \
		$(BLD_OBJ_DIR)/ejsCgi$(BLD_OBJ)

#
#	appweb web server. This will include all the Appweb modules (statically) as they are referenced by appwebLib.
#	It includes a copy of the ejs module and handler (ejsAppweb)
#
$(BLD_BIN_DIR)/ejswebserver$(BLD_EXE): $(patsubst %,$(BLD_OBJ_DIR)/%$(BLD_OBJ),appweb appwebLib ejsAppweb) \
		$(BLD_LIB_DIR)/libejs$(BLD_LIB)
	@bld --exe $(BLD_BIN_DIR)/ejswebserver$(BLD_EXE) --search "$(BLD_APPWEB_LIBPATHS)" \
		--libs "$(BLD_EJS_LIBS)" appweb appwebLib ejsAppweb

#
#	FUTURE
#
apache_install:
	priv apxs -ie mod_ejs.la
	priv apachectl restart

mod_ejs.la: ejsApache.c Makefile
	cp ejsApache.c mod_ejs.c
	apxs -c $(_IFLAGS) -D ENABLE_COMPILE=1 -Wc,"-arch $(BLD_CPU)" -Wl,"-arch $(BLD_CPU)" mod_ejs.c
	cp .libs/mod_ejs.so $(BLD_BIN_DIR)

cleanExtra:
	@rm -f mod_ejs* *.mod */*.mod

#
#	LoadModule ejs_module modules/mod_ejs.so
#	<Location /ejs>
#	SetHandler ejs
#	</Location
#
#	apachectl restart
