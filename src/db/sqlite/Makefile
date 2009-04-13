#
#	Makefile to build Sqlite
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

include 		.makedep

ifeq ($(BLD_HOST_WIN),1)
	MAKE_DFLAGS	+= -D_CRT_SECURE_NO_WARNINGS
endif
_CFLAGS 		:= $(shell echo $(_CFLAGS) | sed 's/-W3/-W1/;s/-Wall/-w/')
