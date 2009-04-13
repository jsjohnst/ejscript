#
#	Makefile -- for the lib directory
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

include		.makedep

ifeq		($(BLD_FEATURE_JAVA_VM),1)
compileExtra: kxml

kxml: ../obj/verified/org/kxml2/io/KXmlParser.class

../obj/verified/org/kxml2/io/KXmlParser.class: kxml2-min-2.3.0.jar
	@for f in kxml*.jar ; \
	do \
		echo cd ../obj/verified \; "$(BLD_HOST_JAR)" xf ../../lib/$$f \; \
			rm -fr META-INF \; cd ../../lib ; \
		cd ../obj/verified ; "$(BLD_HOST_JAR)" xf ../../lib/$$f ; \
			rm -fr META-INF ; cd ../../lib ; \
		echo cd ../obj/classes \; "$(BLD_HOST_JAR)" xf ../../lib/$$f \; \
			rm -fr META-INF \; cd ../../lib ; \
		cd ../obj/classes ; "$(BLD_HOST_JAR)" xf ../../lib/$$f ;  \
			rm -fr META-INF ; cd ../../lib ; \
		find ../obj/classes ../obj/verified -name '*.class' | xargs touch ; \
	done
endif

dependExtra:
	@mkdir -p modules default-web
	@[ "$(BLD_CROSS)" = 1 ] && mkdir -p $(BLD_HOST_SYSTEM) ; true
	@[ "$(BLD_CROSS)" = 1 ] && mkdir -p modules/$(BLD_HOST_SYSTEM) ; true

cleanExtra: cleanFiles

cleanFiles:
	@rm -f libec* libejs* libmpr* libdecimal* *$(BLD_LIB) *.so *.dylib ./$(BLD_HOST_SYSTEM)/* ejsweb.es
	@rm -f ./$(BLD_HOST_SYSTEM)/*$(BLD_LIB)
	@rm -f $(BLD_MOD_DIR)/*.mod $(BLD_MOD_DIR)/*$(BLD_LIB)
	@rm -f mime.types appweb.conf
	@rm -rf default-web 
