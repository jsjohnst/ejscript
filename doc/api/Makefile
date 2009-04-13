#
#	Makefile -- API Documentation makefile
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

include		.makedep

doc: mpr.html ejs.html

mpr.html: Makefile mprBare.html *.tem
	rm -f mpr.html
	cat mprHeader.tem >mpr.html
	cat mprBare.html apiFooter.tem >>mpr.html

ejs.html: Makefile ejsBare.html *.tem
	make -C ejs doc
	rm -f ejs.html
	cat ejsHeader.tem >ejs.html
	cat ejsBare.html apiFooter.tem >>ejs.html

tidy:
	tidy -q -config tidy.conf *.html

dependExtra:
	@chmod +w *.html
