README.TXT for the C sample.
--------------------------------------------------------------------------------

Samples:
    composite  - Composite native class. This is a native class where instance state is stored as a composite
                 C structure. Offers the most optimized memory storage for classes. Ideal for if you have very large
                 numbers of object instances or types with lots of properties. See also the native sample.

    evalFile   - Simple one line Ejscript embedding to evaluate a single script file.

    evalModule - Simple one line Ejscript embedding to evaluate a pre-compiled module file.

    evalScript - Simple one line Ejscript embedding to evaluate a script literal.

    naitve     - Native class. This is a native class that demonstrates binding C functions to JavaScript methods.
                 It creates a loadable module containing the native class. See also the composite sample.

To build:
    make

To run:
    See each sample

Requirements:
    Some samples requires that Ejscript be configured and built with configure --shared. 

--------------------------------------------------------------------------------
Copyright (c) 2003-2009 Embedthis Software, LLC. All Rights Reserved.
Embedthis and AppWeb are trademarks of Embedthis Software, LLC. Other 
brands and their products are trademarks of their respective holders.

See LICENSE.TXT for software license details.
