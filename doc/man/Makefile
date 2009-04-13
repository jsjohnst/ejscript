#
#	Makefile -- Man page makefile
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

include		.makedep

compileExtra: \
	ejsc.txt  ejsweb.txt  ejs.txt  ejsmod.txt  ejsvm.txt  http.txt\
	ejsc.html ejsweb.html ejs.html ejsmod.html ejsvm.html http.html

%.txt: %.1
	man ./$*.1 | sed 's/.\//g' >$*.txt

%.html: %.1
	man ./$*.1 | man2html -u >$*.html

cleanExtra:
	rm -f *.txt *.html
