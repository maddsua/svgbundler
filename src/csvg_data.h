//	2022 maddsua | https://github.com/maddsua/svg-to-css-encoder
//	C svg url encoder

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

#define metaTagsTableItem 32

const char metaTagsTable[][metaTagsTableItem] = {
	 "xmlns:serif","serif:id","data-name"
};

