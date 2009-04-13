#
#	Create a set of text node names for debugging
#

echo -e "char *nodes[] = {\n	\"\","
grep N_ ecAst.h | grep define | sed 's/[ 	]*#define.//' | 
	sed 's/ .*//;s/	//' | sed 's/T_//' | tr '[A-Z]' '[a-z]' |
	sed 's/.*/	"&",/'
echo -e "	0,\n};"
