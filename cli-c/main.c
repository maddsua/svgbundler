//	2022 maddsua | https://github.com/maddsua
//	svg url encoder

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "private.h"

//	comment if not on windows
#include <windows.h>

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


const size_t tableSize = (sizeof(swapTable) / sizeof(urlchar));
const size_t metaTagsTableSize = (sizeof(metaTagsTable) / sizeof(metaTagsTable[0]));


size_t removeMeta(char* srcString, const char* trigger);
char** tdArrPushStr(char** tdArray, size_t* itemsCount, const char* newItem);
unsigned char* loadBinFile(const char* path, size_t* binSize);
void cstrSlideBack(char* str, size_t pos, size_t count);


int main(int argc, char** argv) {
	
	printf("\nSVG to CSS Encoder v%s\n\n", PRODUCT_VERSION);
	
	char** svgFilesList;
	size_t filesToProcess = 0;
	size_t filesDone = 0;
	
	char cssResultFile[PATH_MAX];
		strcpy(cssResultFile, "result.css");
		
	char* cssClassPrefix = NULL;
	
	bool optimize = false;
	bool openInShell = false;
	
	//	help message
	if(argc == 2 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))) {
		printf("Usage: %s -i [svg_file_name.svg] -o [output_file_name.css] -p [css_class_prefix_name]\n -s - enable optimization\n -r - open file in a shell when done\n", ORIGINAL_FILENAME);
		printf("\nTool created by %s\n", LEGAL_COPYRIGHT);
		return 0;
	}
	
	//	go though start flags
	for(int i = 1; i < argc; i++) {
		//	optimize flag
		if(!strcmp(argv[i], "-s")) {
			optimize = true;			
			continue;
		}
		//	open after done flag
		else if(!strcmp(argv[i], "-r")) {
			openInShell = true;			
			continue;
		}
		//	handle multiple args
		if((argc - i) > 1) {
			//	input file
			if(!strcmp(argv[i], "-i")) {
				svgFilesList = tdArrPushStr(svgFilesList, &filesToProcess, argv[i + 1]);
				i++;	
			}
			//	output file
			else if (!strcmp(argv[i], "-o")) {
				strcpy(cssResultFile, argv[i + 1]);
				i++;
			}
			//	prefix select
			else if (!strcmp(argv[i], "-p")) {
				cssClassPrefix = argv[i + 1];
				i++;
			}
		}
	}
	

//	check svg source files
	if(filesToProcess < 1) {
	//	scan directory (only for M$ Шіндошс)
	#ifdef _WINDOWS_
		//	look for .svg
		WIN32_FIND_DATA data = {0};
    	HANDLE hFind = FindFirstFile("*.svg", &data);
		
		//	if there are such a files
		if(hFind != INVALID_HANDLE_VALUE){
			
			svgFilesList = tdArrPushStr(svgFilesList, &filesToProcess, data.cFileName);
			
			//	if there are more of them
			while(FindNextFile(hFind, &data)) {
				svgFilesList = tdArrPushStr(svgFilesList, &filesToProcess, data.cFileName);
			}
		
			FindClose(hFind);
			
		} else {
			printf("No SVG files found. Use --help for more\n");
			return 0;
		}
	#endif
	#ifndef _WINDOWS_
		printf("No SVG files specified. Use --help for more\n");
		return 0;
	#endif
	}
	
//	some more text
		printf("Found %i svg files\n", filesToProcess);
		
		if(optimize) printf(" * SVG optimization enabled\n");
		
		printf("\n");

	

//	create output file
	FILE* convertedCSS = fopen(cssResultFile, "w");
	
		if(convertedCSS == NULL) {
			printf("\n !!! Cannot write css. Filesystem error?\n");
			return 0;
		}

		
//	file processing loop
	for(size_t i_fp = 0; i_fp < filesToProcess; i_fp++) {
		
		//	load svg-source file
		size_t fileLength;
		char* svgContents = loadBinFile(svgFilesList[i_fp], &fileLength);
			if(svgContents == NULL) {
				printf("Can't open \"%s\". Skipping\n", svgFilesList[i_fp]);
				continue;
			}
			
		//	remove any tabs ('\t') so next step will work 100% of the time
		for(size_t i_tr = 0; i_tr < fileLength; i_tr++) {
			if(svgContents[i_tr] == '\t') svgContents[i_tr] = ' ';
		}
		

		//	remove meta tags and calc size difference
		size_t reducedLen = fileLength;
		if(optimize) {
			for(int i_mt = 0; i_mt < metaTagsTableSize; i_mt++) {
				const size_t sizeDiff = removeMeta(svgContents, metaTagsTable[i_mt]);
				if(sizeDiff > 0) {
					i_mt--;
					reducedLen -= sizeDiff;
				}
			}
		}
		float optimizedPercent = ((float)(fileLength - reducedLen) / fileLength) * 100;

		//	updating file length
		if(reducedLen < fileLength) fileLength = reducedLen;

		// remove repeating whitespaces and any line breaks
		for(size_t i_rm = 0; i_rm < fileLength; i_rm++) {
		
			const size_t lastChar = (fileLength - 1);
			const size_t next = (i_rm + 1);
		
			if(svgContents[0] == ' ' || svgContents[0] == '\n') {
				cstrSlideBack(svgContents, 0, 1);
				i_rm--;
			}
			else if(svgContents[lastChar] == ' ' || svgContents[lastChar] == '\n') {
				svgContents[lastChar] = '\0';
				i_rm--;
			}
			else if((svgContents[i_rm] == ' ' && (svgContents[next] == ' ' || svgContents[next] == '<' || svgContents[next] == '>')) || svgContents[i_rm] == '\n') {
				cstrSlideBack(svgContents, i_rm, 1);
				i_rm -= 2;
			}
		}
		
	
		// swap unsafe characters with ok ones
		const size_t plainLen = fileLength;
		const size_t urlEncodedSize = ((3 * plainLen) + 1);
		char* urlEncoded = (char*)malloc(urlEncodedSize);
			memset(urlEncoded, 0, urlEncodedSize);
		size_t i_enc = 0;
		for(size_t i_ss = 0; i_ss < plainLen; i_ss++) {

			bool chSwap = false;
		
			for(int i_sw = 0; i_sw < tableSize; i_sw++) {
				if(svgContents[i_ss] == swapTable[i_sw].from) {
					strcat(urlEncoded, swapTable[i_sw].to);
					i_enc += strlen(swapTable[i_sw].to);
				
					chSwap = true;
					break;
				}
			}

			if(!chSwap) {
				urlEncoded[i_enc] = svgContents[i_ss];
				i_enc++;
			}

		}


		//	report and write to local file
		char filename[PATH_MAX + 1] = {0};
			size_t fnbody = PATH_MAX;
			
			//	remove extension
			char* ifDot = strchr(svgFilesList[i_fp], '.');
			if(ifDot != NULL) {
				fnbody = (size_t)(ifDot - svgFilesList[i_fp]);
			}
			strncpy(filename, svgFilesList[i_fp], fnbody);
			
			// remove white spaces
			for(int i_nm = 0; i_nm < strlen(filename); i_nm++) {
				
				if(filename[i_nm] == ' ') {
					cstrSlideBack(filename, i_nm, 1);
					i_nm--;
				}
			}
		
		
		if(cssClassPrefix != NULL) {
			fprintf(convertedCSS, "\n.%s.%s {\n\tbackground-image: url(\"data:image/svg+xml,%s\");\n}\n", cssClassPrefix, filename, urlEncoded);
		} else {
			fprintf(convertedCSS, "\n.%s {\n\tbackground-image: url(\"data:image/svg+xml,%s\");\n}\n", filename, urlEncoded);
		}
		
		filesDone++;
		
		if(optimize) printf(" --> \"%s\" done (-%2.2f%%)\n", svgFilesList[i_fp], optimizedPercent);
			else printf(" --> \"%s\" done\n", svgFilesList[i_fp]);	
		
		free(urlEncoded);
		free(svgContents);
	}
	
	fclose(convertedCSS);
	
	if(filesDone > 0) {
		
		if(cssClassPrefix != NULL) {
			printf("\nSaved to \"%s\" with a \".%s\" class\n", cssResultFile, cssClassPrefix);
		} else {
			printf("\nSaved to \"%s\"\n", cssResultFile);
		}
		
		if(openInShell) {
			system(cssResultFile);
		}
		
	} else {
		printf("\nNothing is done.\n", cssResultFile);
		remove(cssResultFile);
	}
	
	//	free memory
	for(size_t i = 0; i < filesToProcess; i++) {
		free(svgFilesList[i]);
	}
	free(svgFilesList);
	
	return 0;
}



size_t removeMeta(char* srcString, const char* trigger) {
	
	//	find unwanted meta attribute
	char tagSequence[metaTagsTableItem + 4];
		sprintf(tagSequence, " %s=", trigger);
		
	char* tTagPos = strstr(srcString, tagSequence);
	
	//	if trigger's found
	if(tTagPos != NULL) {

		const size_t origStrLen = strlen(srcString);
		size_t removalSize = 0;
				
		//	get trigger's position in a string
		const size_t seqStart = ((size_t)(tTagPos - srcString));

		
		//	detect attribute start marker: check if there is a ['] or ["] symbol after it
		char attrMarker = srcString[seqStart + strlen(tagSequence)];
		if(attrMarker == '\"' || attrMarker == '\'') {

			for(size_t i = (seqStart + strlen(tagSequence) + 1); i < origStrLen; i++) {

				//	find where unwanted substring ends and cut it
				if(srcString[i] == attrMarker) {

					removalSize = (i - seqStart + 1);

					size_t tailLen = (origStrLen - i);
					char* strTail = (char*)malloc(tailLen + 1);
						strTail[tailLen] = '\0';

						strncpy(strTail, srcString + (i + 1), tailLen);
						srcString[seqStart] = 0;

						strcat(srcString, strTail);

					free(strTail);
					break;
				}
			}
			
			// yes, we've removed that meta data
			return removalSize;
		}
	}

	//	no, there was no such a meta attribute
	return 0;
}

//	push_back() but for 2d array
char** tdArrPushStr(char** tdArray, size_t* itemsCount, const char* newItem) {
	
	const size_t itemLen = strlen(newItem);
	const size_t itemSize = ((itemLen + 1) * sizeof(char));
	
	if(*itemsCount > 0 && tdArray != NULL) {
		
		const size_t oldItems = *itemsCount;
		*itemsCount += 1;

		char** newPool = (char**)realloc(tdArray, *itemsCount * sizeof(char*));
		
			for(int i = oldItems; i < *itemsCount; i++) {
				
				newPool[i] = (char*)malloc(itemSize);
				newPool[i][itemLen] = '\0';
			}
		
		strcpy(newPool[*itemsCount - 1], newItem);

		return newPool;
	}
	else {
		
		*itemsCount = 1;
		
		char** newPool = (char**)malloc(*itemsCount * sizeof(char*));
			newPool[0] = (char*)malloc(itemSize);
			newPool[0][itemLen] = '\0';
		
		strcpy(newPool[0], newItem);
		
		return newPool;
	}
}


//	dynamically loads a binary file to memory
unsigned char* loadBinFile(const char* path, size_t* binSize) {
	
	const short chunkSize = 16384;
	size_t capacity = chunkSize;
	size_t utilized = 0;

	unsigned char* binfileContents = (unsigned char*)malloc(capacity);

	FILE* binaryFile = fopen(path, "rb");
		if(binaryFile == NULL) {
			return NULL;
		}
	
	while(!feof(binaryFile)) {		
		utilized += fread(binfileContents + utilized, 1, chunkSize, binaryFile);
			if(utilized >= capacity) {
				capacity += chunkSize;
				char* tchunk = (char*)realloc(binfileContents, capacity);
				binfileContents = tchunk;
			}
	}
	fclose(binaryFile);
	
	*binSize = utilized;
	return binfileContents;
}

//	analog of .erase() in c++. it erases [count] charasters from [str] starting from [pos]
void cstrSlideBack(char* str, size_t pos, size_t count) {
	
	const size_t cutout = (pos + count);
	const size_t tailLen = (strlen(str) - cutout);
	
	char* tail = (char*)malloc((tailLen + 1) * sizeof(char));
		strncpy(tail, str + cutout, tailLen);
		tail[tailLen] = '\0';
	
	str[pos] = 0;	
	strcat(str, tail);
	
	free(tail);
}
