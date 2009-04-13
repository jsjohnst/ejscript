#
#	Create a set of text token names for debugging
#

echo -e "char *tokenNames[] = {\n	\"\","
grep '[ 	]T_' ecParser.h | grep -v undef | sed 's/#define.//' | 
	sed 's/ .*//;s/	//' | sed 's/T_//' | tr '[A-Z]' '[a-z]' |
	sed 's/.*/	"&",/'
echo -e "	0,\n};"
