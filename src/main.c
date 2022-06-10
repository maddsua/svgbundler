// https://gitlab.com/maddsua/svg-to-css-encoder

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define PLATFORM_WIN32

#ifdef PLATFORM_WIN32

	#include <windows.h>
	
#endif

#include "main.h"
#include "csvg_private.h"


int main(int argc, char** argv) {
	
	printf("\nSVG to CSS Encoder build %i\n\n", VER_BUILD);
	
	char** svgFilesList;
	size_t filesToProcess = 0;
	size_t filesDone = 0;
	
	char cssResultFile[PATH_MAX];
		strcpy(cssResultFile, "result.css");
		
	char* cssClassPrefix = NULL;
	
	bool optimize = false;
	
	
//	start args
	if(argc == 2 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))) {
		printf("Usage: %s -i [svg_file_name.svg] -o [output_file_name.css] -p [css_class_prefix_name]\n -s - enable optimization\n", ORIGINAL_FILENAME);
		printf("\nTool created by %s\n", LEGAL_COPYRIGHT);
		return 0;
	}
	
	// go though flags
	for(int i = 1; i < argc; i++) {
		
		if(!strcmp(argv[i], "-s")) {
			
			optimize = true;			
			continue;
		}
		
		if((argc - i) > 1) {
			
			const size_t nextArgLen = strlen(argv[i + 1]);
			
			if(nextArgLen > 0) {
				
				if(!strcmp(argv[i], "-i")) {
			
					svgFilesList = addFileListItem(svgFilesList, &filesToProcess, argv[i + 1]);
					i++;	
				}
				else if (!strcmp(argv[i], "-o")) {
			
					strcpy(cssResultFile, argv[i + 1]);
					i++;
				}
				else if (!strcmp(argv[i], "-p")) {
					cssClassPrefix = argv[i + 1];
					i++;
				}
			}
		}
	}
	

//	check svg sources
	if(filesToProcess < 1) {
	#ifdef PLATFORM_WIN32
		WIN32_FIND_DATA data = {0};
    	HANDLE hFind = FindFirstFile("*.svg", &data);
		
		if(hFind != INVALID_HANDLE_VALUE){
			
			svgFilesList = addFileListItem(svgFilesList, &filesToProcess, data.cFileName);
    	
			while(FindNextFile(hFind, &data)) {
				svgFilesList = addFileListItem(svgFilesList, &filesToProcess, data.cFileName);
			}
		
			FindClose(hFind);
			
		} else {
			printf("No SVG files found. Use --help for more\n");
			return 0;
		}
	#endif
	#ifndef PLATFORM_WIN32
		printf("No SVG files specified. Use --help for more\n");
		return 0;
	#endif
	}
	
//	some more text
		printf("Found %i svg files\n", filesToProcess);
		
		if(optimize) printf("SVG optimization enabled\n");
		
		printf("\n");

	

//	create output file
	FILE* convertedCSS = fopen(cssResultFile, "w");
	
		if(convertedCSS == NULL) {
			printf("\n !!! Cannot write css. Filesystem error?\n");
			return 0;
		}

		
//	file processing loop
	for(int i = 0; i < filesToProcess; i++) {
		
		//	load svg-source file by chunks
		unsigned int chunks = 1;
		size_t svgContentsSize = (((chunkSize * chunks) + 1) * sizeof(char));
	
		char* svgContents = (char*)malloc(svgContentsSize);
			memset(svgContents, 0, svgContentsSize);
	
		FILE* svgSource = fopen(svgFilesList[i], "r");
	
		if(svgSource == NULL) {
			printf("Can't open \"%s\". Skipping\n", svgFilesList[i]);
			continue;
		}
		
		//	read svg-source file
		while(!feof(svgSource)) {
		
			char* chunk = (char*)malloc((chunkSize + 1) * sizeof(char));
				memset(chunk, 0, (chunkSize + 1) * sizeof(char));
						
			fgets(chunk, chunkSize, svgSource);
		
			unsigned int capacity = (chunkSize * chunks);
			unsigned int utilized = strlen(svgContents);
			if(capacity - utilized < chunkSize) {
			
				chunks++;
				capacity = (chunkSize * chunks);
			
				char* tchunk = (char*)realloc(svgContents, capacity);
			
					if(tchunk == NULL) {
						free(tchunk);
						printf("\n !!! Memory allocation error. Or it's my buggy code at fault, or your computer is fucked.\n");
						return 0;
					}
					svgContents = tchunk;
					
			
				memset(svgContents + utilized, 0, capacity - utilized);
			}
					
			strcat(svgContents, chunk);
			free(chunk);
		}
		
		fclose(svgSource);
		
	
		// remove repeating whitespaces and line breaks
		unsigned int i_rm = 0;
		while(i_rm < strlen(svgContents)) {
			
			const unsigned int lastChar = (strlen(svgContents) - 1);
			
			if(svgContents[0] == ' ' || svgContents[0] == '\n') {
				slideBack(svgContents, 0);
			}
			else if(svgContents[0] == ' ' || svgContents[0] == '\n') {
				svgContents[lastChar] = '\0';
			}
			else if((svgContents[i_rm] == ' ' && svgContents[i_rm + 1] == ' ') || svgContents[i_rm] == '\n') {
				slideBack(svgContents, i_rm);
			}
			else {
				i_rm++;
			}
		}
		
	
		// swap unsafe characters with ok ones
		for(int m = 0; m < strlen(svgContents); m++){
		
			for(int n = 0; n < tableSize; n++){
			
				if(svgContents[m] == swapTable[n].from){
				
					svgContents = swapFor(svgContents, swapTable[n].to, m);
					m += (strlen(swapTable[n].to) - 1);
				
					break;
				}
			}
		}

		//	report and write to file
		char filename[PATH_MAX + 1] = {0};
			size_t fnbody = PATH_MAX;
			
			char* ifDot = strchr(svgFilesList[i], '.');
			
			if(ifDot != NULL) {
				
				size_t dotPos = (size_t)(ifDot - svgFilesList[i] + 1);
				
				fnbody = (dotPos - 1);
			}
		
			strncpy(filename, svgFilesList[i], fnbody);
		
		
		if(cssClassPrefix != NULL) {
			fprintf(convertedCSS, "\n.%s.%s {\n\tbackground-image: url(\"data:image/svg+xml,%s\");\n}\n", cssClassPrefix, filename, svgContents);
		} else {
			fprintf(convertedCSS, "\n.%s {\n\tbackground-image: url(\"data:image/svg+xml,%s\");\n}\n", filename, svgContents);
		}
		
		filesDone++;
		printf(" --> \"%s\" done\n", svgFilesList[i]);
		
		free(svgContents);
	}
	
	fclose(convertedCSS);
	
	if(filesDone > 0 && cssClassPrefix != NULL){
		printf("\nSaved to \"%s\" with a \".%s\" class\n", cssResultFile, cssClassPrefix);
	}
	else if (filesDone > 0) {
		printf("\nSaved to \"%s\"\n", cssResultFile);
	} else {
		printf("\nNothing is done.\n", cssResultFile);
		remove(cssResultFile);
	}
	
	return 0;
}


char** addFileListItem(char** pool, size_t* items, const char* item) {
	
	const size_t itemLen = ((PATH_MAX + 1) * sizeof(char));
	
	if(*items > 0 && pool != NULL) {
		
		const size_t oldItems = *items;
		*items += 1;

		char** newPool = (char**)realloc(pool, *items * sizeof(char*));
			for(int i = oldItems; i < *items; i++) {
				newPool[i] = (char*)malloc(itemLen);
				memset(newPool[i], 0, itemLen);
			}
		
		strncpy(newPool[*items - 1], item, PATH_MAX);

		return newPool;
	}
	else {
		
		*items = 1;
		
		char** newPool = (char**)malloc(*items * sizeof(char*));
			newPool[0] = (char*)malloc(itemLen);
			memset(newPool[0], 0, itemLen);
		
		strncpy(newPool[0], item, PATH_MAX);
		
		return newPool;
	}
}

void rmtd(char** array, const size_t x) {
	
	for(int i = 0; i < x; i++) {
		free(array[i]);
	}
	free(array);
}

char* swapFor(char* str, const char* substr, size_t pos) {
	
	// slice
	size_t pastLen = pos + 1;
	size_t pastSize = ((pastLen + 1) * sizeof(char));
	char* past = (char*)malloc(pastSize);
	
		memset(past, 0, pastSize);
		strncpy(past, str, pastLen - 1);
	
	size_t leftLen = (strlen(str) - (pos + 1));
	size_t leftSize = ((leftLen + 1) * sizeof(char));
	char* left = (char*)malloc(leftSize);
	
		memset(left, 0, leftSize);
		strncpy(left, str + pastLen, leftLen);
			
	// combine and return	
	size_t resultLen = pastLen + leftLen + strlen(substr);
	size_t resultSize = ((resultLen + 1) * sizeof(char));
		
	char* swapResult = (char*)realloc(str, resultSize);
			
		if(swapResult == NULL) {
			free(swapResult);
			swapResult = (char*)malloc(resultSize);
		}
	
		memset(swapResult, 0, resultSize);
		strcat(swapResult, past);
		strcat(swapResult, substr);
		strcat(swapResult, left);

	free(past);
	free(left);
						
	return swapResult;
}

void slideBack(char* str, size_t pos) {
	
	size_t baseLen = pos + 1;
	size_t baseSize = ((baseLen + 1) * sizeof(char));
	char* base = (char*)malloc(baseSize);
		
		memset(base, 0, baseSize);
		strncpy(base, str, baseLen - 1);
		
	size_t shiftLen = (strlen(str) - (pos + 1));
	size_t shiftSize = ((shiftLen + 1) * sizeof(char));
	char* shift = (char*)malloc(shiftSize);
	
		memset(shift, 0, shiftSize);
		strncpy(shift, str + baseLen, shiftLen);
		
	// put together
	memset(str + pos, 0, shiftLen);
	strcat(str, shift);
	//puts(str);
	free(base);
	free(shift);
}

/*
	
	puts("\n\n");
	
	char findtest[] = "123456789";
	
	int position = (int)(strchr(findtest, '5') - findtest + 1);
	printf("positiong is %i", position);*/
	
	
