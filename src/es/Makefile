#
#	Makefile for the Ejscript script code. 
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

BUILD_CROSS	= 0

include 	.makedep

#
#	Order does influence slot allocation. Put the core types first.
#
CORE_FILES	:= \
	core/Object.es \
	core/Boolean.es \
	core/Null.es \
	core/Number.es \
	core/String.es \
	core/Void.es \
	core/Function.es \
	core/Array.es \
	core/Block.es \
	core/ByteArray.es \
	core/Date.es \
	core/Error.es \
	core/Iterator.es \
	core/JSON.es \
	core/Math.es \
	core/Name.es \
	core/Namespace.es \
	core/Reflect.es \
	core/RegExp.es \
	core/Stream.es \
	core/Type.es \
	core/Global.es \
	events/Event.es \
	events/Dispatcher.es \
	events/Timer.es \
	xml/XML.es \
	xml/XMLList.es \
	io/BinaryStream.es \
	io/File.es \
	io/Http.es \
	io/Socket.es \
	io/TextStream.es \
	io/Url.es \
	io/XMLHttp.es \
	sys/App.es \
	sys/Config.es \
	sys/Debug.es \
	sys/GC.es \
	sys/Logger.es \
	sys/Memory.es \
	sys/System.es \
	sys/Unix.es

DB_FILES	:= \
	db/Database.es \
	db/Record.es

ifeq	($(BLD_FEATURE_EJS_WEB),1)
	WEB_FILES	:= \
		web/Controller.es	\
		web/Cookie.es	\
		web/Host.es	\
		web/Request.es	\
		web/Response.es	\
		web/Session.es	\
		web/UploadFile.es	\
		web/View.es \
		web/connectors/HtmlConnector.es \
		web/connectors/GoogleConnector.es
endif

CORE_TARGETS	+= $(BLD_MOD_DIR)/ejs.mod
CORE_HEADERS 	+= $(BLD_INC_DIR)/ejs.slots.h
CORE_HEADERS 	+= $(BLD_INC_DIR)/ejs.events.slots.h
CORE_HEADERS 	+= $(BLD_INC_DIR)/ejs.io.slots.h
CORE_HEADERS 	+= $(BLD_INC_DIR)/ejs.sys.slots.h

ifeq	($(BLD_FEATURE_STATIC),1)
	#
	#	Static builds put ejs.db and ejs.web into the core
	#
	CORE_FILES	+= $(DB_FILES) $(WEB_FILES)
	DB_FILES	=
	WEB_FILES	=
	_ESFLAGS	+= --bind
	ifeq	($(BLD_FEATURE_EJS_DB),1)
		CORE_HEADERS	+= $(BLD_INC_DIR)/ejs.db.slots.h
	endif
	ifeq	($(BLD_FEATURE_EJS_WEB),1)
		CORE_HEADERS	+= $(BLD_INC_DIR)/ejs.web.slots.h
	endif
else
	ifeq	($(BLD_FEATURE_EJS_DB),1)
		EXTRA_TARGETS	+= $(BLD_MOD_DIR)/ejs.db.mod
		EXTRA_HEADERS	+= $(BLD_INC_DIR)/ejs.db.slots.h
	endif
	ifeq	($(BLD_FEATURE_EJS_WEB),1)
		EXTRA_TARGETS	+= $(BLD_MOD_DIR)/ejs.web.mod
		EXTRA_HEADERS	+= $(BLD_INC_DIR)/ejs.web.slots.h
	endif
endif
ifeq	($(BLD_FEATURE_EJS_WEB),1)
	ifneq ($(BUILDING_CROSS),1) 
		EXTRA_TARGETS	+= $(BLD_MOD_DIR)/ejsweb.mod
	endif
endif

# EXTRA_TARGETS			+= file.list

ifeq	($(BLD_FEATURE_DOC),1)
	DOC_DIR				:= $(BLD_TOP)/doc/api/ejscript
	EXTRA_TARGETS		+= $(DOC_DIR)/index.html
endif

ifeq	($(BLD_DEBUG),0)
#
#	Currently all builds are debug. TODO - remove this eventually
#
_ESFLAGS	+= --debug
endif

compileExtra: $(CORE_TARGETS) $(CORE_HEADERS)

modules: $(EXTRA_TARGETS) $(EXTRA_HEADERS) dummy

#
#	Only copy the slot headers if they actually change. Minimizes build churn.
#
$(BLD_INC_DIR)/%.slots.h: %.slots.h
	@if ! diff $@ $< >/dev/null 2>&1 ; then \
		echo cp $< $@ ; \
		cp $< $@ ; \
	fi

$(BLD_MOD_DIR)/ejs.mod: ejs.mod
	cp ejs.mod $(BLD_MOD_DIR)/ejs.mod

ejs.mod: $(CORE_FILES) $(BLD_TOP)/buildConfig.make
	$(call setlibpath) ; ejsc $(_ESFLAGS) --bind --empty --out ejs.mod $(CORE_FILES)

ejs.slots.h: ejs.mod
	$(call setlibpath) ; ejsmod --showDebug --empty --showBuiltin --listing --cslots ejs.mod

#
#	Db
#
ejs.db.mod: $(DB_FILES) $(CORE_FILES) $(BLD_TOP)/buildConfig.make
	$(call setlibpath) ; ejsc $(_ESFLAGS) --out ejs.db.mod $(DB_FILES)

$(BLD_MOD_DIR)/ejs.db.mod: ejs.db.mod
	cp ejs.db.mod $(BLD_MOD_DIR)/ejs.db.mod

ifeq	($(BLD_FEATURE_STATIC),0)
ejs.db.slots.h: ejs.db.mod
	$(call setlibpath) ; ejsmod --listing --cslots ejs.db.mod
endif

#
#	Web
#
ejs.web.mod: $(WEB_FILES) $(CORE_FILES) $(BLD_TOP)/buildConfig.make
	$(call setlibpath) ; ejsc $(_ESFLAGS) --out ejs.web.mod $(WEB_FILES)

$(BLD_MOD_DIR)/ejs.web.mod: ejs.web.mod
	cp ejs.web.mod $(BLD_MOD_DIR)/ejs.web.mod

ifeq	($(BLD_FEATURE_STATIC),0)
ejs.web.slots.h: ejs.web.mod
	$(call setlibpath) ; ejsmod --listing --cslots ejs.web.mod
endif

#
#	Generate documentation for all es files
#	
doc: $(BLD_TOP)/doc/api/ejscript/index.html

$(DOC_DIR)/index.html: $(CORE_FILES) $(DB_FILES) $(WEB_FILES)
	$(call setlibpath) ; ejsc --doc --bind $(_ESFLAGS) --out doc.mod --empty $(CORE_FILES) $(DB_FILES) $(WEB_FILES)
	rm -rf $(DOC_DIR)/*.html $(DOC_DIR)/*.css $(DOC_DIR)/images/*
	$(call setlibpath) ; ejsmod --html $(DOC_DIR) --empty --showBuiltin doc.mod
	rm -f doc.mod

#
#	Create the ejsweb module
#
$(BLD_MOD_DIR)/ejsweb.mod: web/ejsweb.es
	$(call setlibpath) ; ejsc --out $(BLD_MOD_DIR)/ejsweb.mod web/ejsweb.es
	@mkdir -p $(BLD_LIB_DIR)/default-web/images
	@mkdir -p $(BLD_LIB_DIR)/default-web/themes
	cp -r web/default-web/* $(BLD_LIB_DIR)/default-web

cleanExtra:
	rm -f *.h *.mod *.java *.lst $(BLD_MOD_DIR)/ejsweb.mod 
