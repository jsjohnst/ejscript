README.TXT for the ejs directory.
--------------------------------------------------------------------------------

This directory contains the Ejscript script source code for the system types 
and support libraries. 

The system type files are interface specifications that the compiler will 
parse to generate property slot defintion files for the C and Java VMs.

Ejs Documentation
--------------------------------------------------------------------------------

The Ejs compiler will parse documentation comments within the /** comment */ comment
delimiters. The first line of text is assumed to be the brief overview. The rest of
the text is the documentation description.

Ejs documentation also supports the following documentation directives:

	@param	argName Description 		(Up to next @, case matters on argName)
	@return	Sentance					(If sentance starts with lower case, then start sentance with "Call returns")
	@throws ExceptionType Explanation 	(Explanation is the text up to the next @)
	@see keyword keyword ...			(Hot linked see-also keywords)
	@example Description			    (Description is up to next @)
	@stability kind						(Kinds: prototype | evolving | stable | mature | deprecated]
	@requires ECMA 						(Will emit: configuration requires --ejs-ecma)
	@spec								(ecma-262, ecma-357, ejs-11)
	@hide								(Hides this entry. API exists but is not documented)


--------------------------------------------------------------------------------
Copyright (c) 2003-2009 Embedthis Software, LLC. All Rights Reserved.
Embedthis and AppWeb are trademarks of Embedthis Software, LLC. Other 
brands and their products are trademarks of their respective holders.

See LICENSE.TXT for software license details.
