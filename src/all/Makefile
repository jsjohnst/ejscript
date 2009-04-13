#
#	Makefile for Ejscript
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#
#	This is an all-in-one build where all the Ejscript source files are catenated into as few files as possible. 
#	See http://hg.embedthis.com/ejs if you require the full source for the Ejscript.
#

include 		.makedep

ifeq ($(BLD_EJS_DIR),)
	EJS_OBJECTS		+= ejsLib
	EC_OBJECTS		+= ecLib

	ifeq ($(BLD_FEATURE_SQLITE),1)
		TARGETS		+= $(BLD_LIB_DIR)/libsqlite3$(BLD_LIB)
		TARGETS		+= $(BLD_BIN_DIR)/ajssql$(BLD_EXE)
	endif

	TARGETS			+= $(BLD_LIB_DIR)/libejs$(BLD_LIB)
	TARGETS			+= $(BLD_LIB_DIR)/libec$(BLD_LIB)
	TARGETS			+= $(BLD_LIB_DIR)/libejsgate$(BLD_LIB)
	TARGETS			+= $(BLD_BIN_DIR)/ajsc$(BLD_EXE) $(BLD_BIN_DIR)/ajs$(BLD_EXE) $(BLD_BIN_DIR)/ajsmod$(BLD_EXE)
	ifeq ($(BLD_FEATURE_EJS_WEB),1)
		TARGETS		+= $(BLD_BIN_DIR)/ajsweb$(BLD_EXE)
		ifeq	($(BLD_FEATURE_STATIC),1)
			ifeq	($(BLD_FEATURE_APPWEB),1)
				GATE_OBJECTS += ejsAppweb$(BLD_OBJ)
			endif
		endif
		TARGETS		+= $(BLD_MOD_DIR)/ajs.mod
		TARGETS		+= $(BLD_MOD_DIR)/ajsweb.mod
	endif
endif

DOC_DIR			:= $(BLD_TOP)/doc/api/ejscript

#
#	Suppress warnings for sqlite
#
_CFLAGS 		:= $(shell echo $(_CFLAGS) | sed 's/-W3/-W1/;s/-Wall/-w/')

compileExtra: $(TARGETS)

#
#	Modules can only be built after the appweb library is built
#
modules: $(MODULES) dummy

$(BLD_MOD_DIR)/ajs.mod: $(BLD_INC_DIR)/ejs.slots.h *.es $(BLD_BIN_DIR)/ajsc$(BLD_EXE) $(BLD_BIN_DIR)/ajsmod$(BLD_EXE)
	$(call setlibpath) ; ajsc $(_ESFLAGS) --bind --empty --out $(BLD_MOD_DIR)/ajs.mod ejs.es
	$(call setlibpath) ; ajsmod --showDebug --listing --empty --showBuiltin --cslots $(BLD_MOD_DIR)/ajs.mod
	cat ejs*.slots.h >.ejs.slots.h
	@if ! diff .ejs.slots.h $(BLD_INC_DIR)/ejs.slots.h >/dev/null 2>&1 ; then \
		chmod +w $(BLD_INC_DIR)/ejs.slots.h ; \
		cp .ejs.slots.h $(BLD_INC_DIR)/ejs.slots.h ; \
		touch .ejs.slots.h ; \
		make compile; [ $$? != 0 ] && exit 255 ; true ; \
	else true ; \
	fi

#
#	Build the ejs, ejsc, gateway and sqlite libraries
#	
$(BLD_LIB_DIR)/libejs$(BLD_LIB): $(OBJECTS) $(BLD_LIB_DIR)/libmpr$(BLD_LIB)
	@bld --library $(BLD_LIB_DIR)/libejs --search "$(BLD_EJS_WITHPATHS)" --libs "$(BLD_EJS_WITHLIBS)" $(EJS_OBJECTS)

$(BLD_LIB_DIR)/libec$(BLD_LIB): $(OBJECTS) $(BLD_LIB_DIR)/libmpr$(BLD_LIB) $(BLD_LIB_DIR)/libejs$(BLD_LIB)
	@bld --library $(BLD_LIB_DIR)/libec --search "$(BLD_EJS_WITHPATHS)" --libs "$(BLD_EJS_WITHLIBS) ejs" $(EC_OBJECTS)

$(BLD_LIB_DIR)/libejsgate$(BLD_LIB): $(OBJECTS) $(BLD_LIB_DIR)/libmpr$(BLD_LIB) $(BLD_LIB_DIR)/libejs$(BLD_LIB)
	@bld --library $(BLD_LIB_DIR)/libejsgate --search "$(BLD_EJS_WITHPATHS)" --libs "$(BLD_EJS_WITHLIBS) ejs" \
		$(GATE_OBJECTS)

$(BLD_LIB_DIR)/libsqlite3$(BLD_LIB): $(OBJECTS) $(BLD_LIB_DIR)/libmpr$(BLD_LIB)
	@bld --library $(BLD_LIB_DIR)/libsqlite3 --search "$(BLD_SQLITE_WITHLIBS)" --libs "$(BLD_SQLITE_WITHLIBS)" sqlite3

$(BLD_BIN_DIR)/ajssql$(BLD_EXE): $(OBJECTS) $(BLD_LIB_DIR)/libmpr$(BLD_LIB)
	@bld --exe $(BLD_BIN_DIR)/ajssql$(BLD_EXE) --search "$(BLD_SQLITE_LIBPATHS)" --libs "$(BLD_SQLITE_LIBS) \
		$(BLD_MPR_LIBS)" ejssql

$(BLD_BIN_DIR)/ajsc$(BLD_EXE): $(OBJECTS) $(BLD_LIB_DIR)/libejs$(BLD_LIB) $(BLD_LIB_DIR)/libmpr$(BLD_LIB)
	@bld --exe $(BLD_BIN_DIR)/ajsc$(BLD_EXE) --search "$(BLD_EJS_LIBPATHS)" --libs "$(BLD_EJS_LIBS) ec" ejsc

$(BLD_BIN_DIR)/ajs$(BLD_EXE): $(OBJECTS) $(BLD_LIB_DIR)/libejs$(BLD_LIB) $(BLD_LIB_DIR)/libmpr$(BLD_LIB)
	@bld --exe $(BLD_BIN_DIR)/ajs$(BLD_EXE) --search "$(BLD_EJS_LIBPATHS)" --libs "$(BLD_EJS_LIBS) ec" ejs

$(BLD_BIN_DIR)/ajsmod$(BLD_EXE): $(OBJECTS) $(BLD_LIB_DIR)/libejs$(BLD_LIB) $(BLD_LIB_DIR)/libmpr$(BLD_LIB)
	@bld --exe $(BLD_BIN_DIR)/ajsmod$(BLD_EXE) --search "$(BLD_EJS_LIBPATHS)" --libs "$(BLD_EJS_LIBS)" ejsmod

$(BLD_BIN_DIR)/ajsweb$(BLD_EXE): $(OBJECTS) $(BLD_LIB_DIR)/libmpr$(BLD_LIB)
	@bld --exe $(BLD_BIN_DIR)/ajsweb$(BLD_EXE) --search "(BLD_EJS_LIBPATHS)" --libs "ec $(BLD_EJS_LIBS)" ejsweb

$(BLD_MOD_DIR)/ajsweb.mod: ejsweb.es
	$(call setlibpath) ; ejsc --bind --out $(BLD_MOD_DIR)/ajsweb.mod ejsweb.es
	@mkdir -p $(BLD_LIB_DIR)/default-web/images
	@mkdir -p $(BLD_LIB_DIR)/default-web/themes
	chmod -R +w $(BLD_LIB_DIR)/default-web
	cp -r default-web/* $(BLD_LIB_DIR)/default-web
	chmod -R +w $(BLD_LIB_DIR)/default-web
	@[ -f $(BLD_LIB_DIR)/appweb.conf ] && chmod 644 $(BLD_LIB_DIR)/appweb.conf ; true
	@[ -f $(BLD_LIB_DIR)/mime.types ] && chmod 644 $(BLD_LIB_DIR)/mime.types ; true
	cp appweb.conf mime.types $(BLD_LIB_DIR)

#
#	Generate documentation for all es files
#	
doc: $(BLD_TOP)/doc/api/ejscript/index.html

$(DOC_DIR)/index.html: Makefile ejs.es
	$(call setlibpath) ; ejsc --doc --out doc.mod --empty --bind ejs.es
	rm -rf $(DOC_DIR)/*.html $(DOC_DIR)/*.css $(DOC_DIR)/images/*
	$(call setlibpath) ; ejsmod --html $(DOC_DIR) --empty --showBuiltin doc.mod
	rm -f doc.mod

#
#	Modules
#
ifeq	($(BLD_FEATURE_APACHE),1)
$(BLD_MOD_DIR)/mod_apache_ejs$(BLD_SHOBJ): $(OBJECTS) $(BLD_LIB_DIR)/libejs$(BLD_LIB) $(BLD_LIB_DIR)/libmpr$(BLD_LIB)
	@bld --library $(BLD_MOD_DIR)/mod_apache_ejs$(BLD_SHOBJ) --search "$(BLD_EJS_LIBPATHS)" \
		--libs "$(BLD_EJS_LIBS)" ejsApache
endif

ifeq	($(BLD_FEATURE_APPWEB),1)
$(BLD_MOD_DIR)/mod_ejs$(BLD_SHOBJ): $(OBJECTS) $(BLD_LIB_DIR)/libejs$(BLD_LIB) $(BLD_LIB_DIR)/libmpr$(BLD_LIB)
	@bld --shared --library $(BLD_MOD_DIR)/mod_ejs --search "$(BLD_APPWEB_LIBPATHS)" \
		--libs "$(BLD_APPWEB_LIBS) $(BLD_EJS_LIBS)" ejsAppweb
endif

