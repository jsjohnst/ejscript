#
#	Makefile for the Ejscript unit test program
#
#	Copyright (c) Embedthis LLC, 2003-2009. All Rights Reserved.
#

BF_LEVEL	?=	1
POST_DIRS	:= bench
TESTS 		+= compiler/*.es parser/*.es core/*.es events/*.es sys/*.es regress/*.es 
TESTS 		+= io/*file*.es io/*textstream*.es

include 	.makedep

ifeq	($(BLD_FEATURE_EJS_E4X),1)
	TESTS	+= xml/*.es 
endif
ifeq	($(BLD_FEATURE_EJS_DB),1)
	TESTS	+= db/*.es 
endif
ifeq	($(BLD_FEATURE_EJS_DOC),1)
	TEST_DOC += testDoc
endif

SEARCH	:= "$(BLD_BIN_DIR)"

testExtra: testCompile testRun testVm testList testSlots $(TEST_DOC)
	
testPrep:
ifeq ($(BLD_FEATURE_EJS_DB),1)
	$(call setlibpath) ; ejssql db/test.sdb < db/test.sql
endif

testAll:
	set -e ; \
	make test EFLAGS="" ; \
	make test EFLAGS="--debug" ; \
	make test EFLAGS="--optimize 1" ; \
	make test EFLAGS="--nobind" ; \

nobind:
	make test EFLAGS="--nobind"

#
#	Just compile
#
testCompile:
	$(call setlibpath) ; bash ./ejsTest.ksh --verbose --testName ejs --compile --args "$(EFLAGS)" $(TESTS)

#
#	Run using ejs
#
testRun: testPrep
	$(call setlibpath) ; bash ./ejsTest.ksh --verbose --testName ejs --run --args "$(EFLAGS)" $(TESTS)

#
#	Run in a stand-alone vm
#
testVm: testPrep
	$(call setlibpath) ; bash ./ejsTest.ksh --verbose --testName ejs --runvm --args "$(EFLAGS)" $(TESTS)

#
#	Generate documentation
#
testDoc:
	$(call setlibpath) ; bash ./ejsTest.ksh --verbose --testName ejs --doc --args "$(EFLAGS)" $(TESTS)

#
#	Generate listing
#
testList:
	$(call setlibpath) ; bash ./ejsTest.ksh --verbose --testName ejs --list --args "$(EFLAGS)" $(TESTS)

#
#	Generate slot files
#
testSlots:
	$(call setlibpath) ; bash ./ejsTest.ksh --verbose --testName ejs --slots --args "$(EFLAGS)" $(TESTS)


valgrindTest:
	@$(call setlibpath) ; if `type valgrind >/dev/null 2>&1` ; then \
		set -e ; \
		bash ./ejsTest.ksh --verbose --testName ejs --valgrind --run $(TESTS) ; \
	fi

buildFarmPrep:
	@echo Doing test at level: $(BF_LEVEL)

buildFarmTestExtra: buildFarmPrep testExtra 
	@if [ "$(BF_LEVEL)" -gt 2 ] ; then \
		make valgrindTest ; \
	fi

#
#	TODO - this is not an ideal load test. Really need an event driven app
#
loadTestExtra:
	@echo -e "\n  # Running load tests for $(BLD_PRODUCT).ejs" 
	@total=5 ; \
	count=0; \
	while [ "$$count" -lt $$total ] ; \
	do \
		$(call setlibpath) ; bash ./ejsTest.ksh --testName ejs --run $(TESTS);\
		status=$$? ; \
		[ "$$status" != 0 ] && exit 1 ; \
		count=`expr $$count + 1` ; \
		echo -e "\n  Completed $$count of $$total iterations" ; \
	done ; \
	true

#
#	TODO - use compiler/* and core/* when fixed
#
leakTestExtra:
	@if `type valgrind >/dev/null 2>&1` ; then \
		echo "  # Running leak test for $(BLD_PRODUCT).ejs" ; \
		leakTest --name $(BLD_PRODUCT).ejs --cmd "$(BLD_BIN_DIR)/ejs parser/00000-empty.es" ; \
		leakTest --name $(BLD_PRODUCT).ejs --cmd "$(BLD_BIN_DIR)/ejs compiler/00180-var.es" ; \
	fi

#
#	BUG - this test fails as not all vars are GC'd before exit
# 
#	@leakTest --name $(BLD_PRODUCT).ejs --cmd "$(BLD_BIN_DIR)/ejs core/01400-string.es"

cleanExtra:
	@rm -f *.lst */*.lst */*.mod *.mod *.h *.java

dependExtra:
	@chmod +x *.ksh
