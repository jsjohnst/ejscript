#
#	Makefile for the packages
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

include 		.makedep 

PRE_DIRS		= $(BLD_HOST_OS)
BASE_NAME		:= "$(BLD_PRODUCT)-$(BLD_VERSION)-$(BLD_NUMBER)-$(BLD_HOST_DIST)-$(BLD_HOST_OS)-$(BLD_HOST_CPU)"
WIN_INS_DIR		:= c:/tmp/testInsAppweb
LAUNCH_BROWSER	:=  1

install-package: prep
	@if [ $(BLD_HOST_OS) = WIN ] ; then \
		name="$(BASE_NAME).exe.zip" ; \
		rm -fr "$(WIN_INS_DIR)" ; \
		cd $(BLD_TOP); top=`pwd` ; cd /tmp ; unzip -o $$top/releases/$(BLD_PRODUCT)/$$name ; \
		name="${BLD_PRODUCT}-${BLD_VERSION}-${BLD_NUMBER}-${BLD_HOST_OS}-${BLD_HOST_CPU}.exe" ; \
		cp $$top/package/WIN/unattended.ans /tmp ; \
		echo "Dir=$(WIN_INS_DIR)" >>/tmp/unattended.ans ; \
		$$name /loadinf=/tmp/unattended.ans /silent ; \
	else \
		name="$(BASE_NAME).tar.tar.gz" ; \
		cd $(BLD_TOP); top=`pwd` ; cd /tmp ; tar xvfz $$top/releases/$$name ; \
		priv "$(BLD_PRODUCT)_HEADLESS=1 ./install" ; \
	fi

uninstall-package: prep
	@if [ $(BLD_HOST_OS) = WIN ] ; then \
		if [ ! -x "$(WIN_INS_DIR)/unins000.exe" ] ; then \
			echo "Can't find $(WIN_INS_DIR)/unins000.exe to uninstall" ; \
			exit 255 ; \
		fi ; \
		"$(WIN_INS_DIR)/unins000.exe" /silent ; \
	else \
		name="$(BASE_NAME).tar.tar.gz" ; \
		cd $(BLD_TOP); top=`pwd` ; cd /tmp ; tar xvfz $$top/releases/$(BLD_PRODUCT)/$$name ; \
		priv "$(BLD_PRODUCT)_HEADLESS=1 ./remove" ; \
	fi

install install-%: prep
	@packs=$* ; [ "$$packs" = "" ] && packs="binary" ; \
	[ "$$packs" = "all" ] && packs="binary dev" ; \
	[ "$$packs" = "src" ] && packs="source" ; \
	priv "export BLD_TOP=$(BLD_TOP) DRY_RUN=$(DRY_RUN) TRACE=$(TRACE) TASK=Install ROOT_DIR="$(ROOT_DIR)" \
		LAUNCH_BROWSER=$(LAUNCH_BROWSER) ; set -e ; $(BLD_TOP)/build/bin/makeInstall $$packs" ; \

uninstall uninstall-%: prep
	@packs=$* ; [ "$$packs" = "" ] && packs="binary" ; \
	[ "$$packs" = "all" ] && packs="binary dev" ; \
	[ "$$packs" = "src" ] && packs="source" ; \
	priv "export BLD_TOP=$(BLD_TOP) DRY_RUN=$(DRY_RUN) TRACE=$(TRACE) TASK=Remove ; export ROOT_DIR="$(ROOT_DIR)" ; \
		set -e ; $(BLD_TOP)/build/bin/makeInstall $$packs" ; \
	
checkInstalled:
	@for d in /etc /usr/src /usr/share /usr/share/doc /usr/lib /usr/include /var/log /var/www "$(BLD_PREFIX)" ; \
	do \
		[ -x "$$d/$(BLD_PRODUCT)" ] && echo "Found $$d/$(BLD_PRODUCT)" || true; \
	done
	@for d in "$(BLD_DOC_PREFIX)" "$(BLD_INC_PREFIX)" "$(BLD_LIB_PREFIX)" "$(BLD_LOG_PREFIX)" "$(BLD_MAN_PREFIX)" \
		"$(BLD_SAM_PREFIX)" "$(BLD_BIN_PREFIX)" "$(BLD_SBIN_PREFIX)" "$(BLD_SRC_PREFIX)" ; \
	do \
		[ -x "$$d/$(BLD_PRODUCT)" ] && echo "Found $$d/$(BLD_PRODUCT)" || true; \
	done

prep:
	@if [ ! -x $(BLD_TOOLS_DIR)/edep$(BLD_BUILD_EXE) -a "$(BUILDING_CROSS)" != 1 ] ; then \
		$(MAKE) -S --no-print-directory _RECURSIVE_=1 -C $(BLD_TOP)/build/src compile ; \
	fi

dependExtra:
	@[ "$(shell echo *.sh)" != "*.sh" ] && chmod +x *.sh ; true
