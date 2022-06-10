//	https://gitlab.com/maddsua/svg-to-css-encoder

typedef struct _urlchar {
	char from;
	char to[4];
} urlchar;
	
const urlchar swapTable[] = {
		'\"', "%22",
		'\'', "%27",
		'\\', "%5C",
		'/', "%2F",
		'>', "%3E",
		'<', "%3C",
		' ', "%20",
		'%', "%25",
		'{', "%7B",
		'}', "%7D",
		'|', "%7C",
		'^', "%5E",
		'`', "%60",
		':', "%3A",
		'\?', "%3F",
		'#', "%23",
		'[', "%5B",
		']', "%5D",
		'@', "%40",
		'!', "%21",
		'$', "%24",
		'&', "%26",
		'(', "%28",
		')', "%29",
		'*', "%2A",
		'+', "%2B",
		',', "%2C",
		';', "%3B",
		'=', "%3D"
};
	

const size_t tableSize = (sizeof(swapTable) / sizeof(urlchar));
const size_t chunkSize = 1024;
const size_t emptyStrSize = (1 * sizeof(char));


void slideBack(char* str, size_t pos);
char* swapFor(char* str, const char* substr, size_t pos);
void rmtd(char** array, const size_t x);
char** addFileListItem(char** pool, size_t* items, const char* item);
