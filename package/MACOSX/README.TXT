Embedthis Ejscript(TM)

Welcome to the Embedthis Ejscript distribution for MAC OS X. 

This document describes the procedure for installing Ejscript. This package contains the Ejscript compiler, virtual machine, and module manager programs. It also contains the Ejscript libraries and program headers necessary to embed Ejscript in an application. Once installed, you can you may read the Ejscript online documentation and samples for further information. You can also read the documentation for the latest release online at:

    http://www.embedthis.com/products/ejscript/doc/index.html

This software is copyrighted and distributed under license. Please read the LICENSE.TXT for details.


System Requirements

To install and run Embedthis Ejscript, your system will need at least the following:

    * MAC OS X 10.4 or later
    * 600 KB spare memory
    * 10 MB disk space
    * TCP/IP network stack

To rebuild Ejscript from source code, you will need at least:

    * 30 MB disk space


Installing Embedthis Ejscript

The installation distribution includes sub-packages containing pre-built binaries, development libraries, headers, documentation and samples. You can selectively install the sub-packages or you can install the complete distribution.

For those looking to build from source code, you will need to download the separate source code distribution. The product documentation describes how to build from source. 


Running Ejscript

The Ejscript product installation includes a command line shell that can be invoked by typing:

	ejs

This command responds with the following prompt:

ejs>

Ejscript commands can be entered and results will be echoed to the console.  See the user documentation for further details.

To compile Ejscript programs for later execution by the stand-alone Ejscript virtual machine, use the "ec" compiler:

    ec file.es file2.es

This will create output modules with a ".mod" extension. These can be run by invoking the Ejscript VM:

    ejsvm default.mod


Building the Source Code

If you wish to rebuild the Ejscript product from the distributed source code, please download the source code distribution from the Embedthis web site. Then read "Building from Source" in the Ejscript product documentation for an explanation of the necessary steps to successfully rebuild the product.


License and Copyright Information

This software is supplied under license. Both a GNU and commercial license are available. Please read LICENSE.TXT for details. This documentation and the software described in this document may be used and copied only in accordance with the terms of the accompanying license agreement. Product and technical information in this document is subject to change without notice and does not represent a commitment on the part of Embedthis Software LLC.

Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.

Embedthis is a trademark of Embedthis Software LLC. Embedthis, Ejscript, and EGI are trademarks of Embedthis Software. All other brand or product names are the trademarks or registered trademarks of their respective holders.


Embedthis Software, LLC.
7427 Orin Court N
Seattle, WA 98103.
+1 (425) 329-3490
www.embedthis.com
info@embedthis.com
