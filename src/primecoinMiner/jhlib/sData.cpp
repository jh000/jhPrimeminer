#include"./JHLib.h"

static char sData_whitespaceList[] = {' ','\t'};

char *streamEx_readLine(stream_t *stream)
{
	uint32 currentSeek = stream_getSeek(stream);
	uint32 fileSize = stream_getSize(stream);
	uint32 maxLen = fileSize - currentSeek;
	if( maxLen == 0 )
		return NULL; // eof reached
	// begin parsing
	char *cstr = (char*)malloc(512);
	sint32 size = 0;
	sint32 limit = 512;
	while( maxLen )
	{
		maxLen--;
		char n = stream_readS8(stream);
		if( n == '\r' )
			continue; // skip
		if( n == '\n' )
			break; // line end
		cstr[size] = n;
		size++;
		if( size == limit )
		{
			OutputDebugString("sData: Buffer overrun detected");
			ExitProcess(-4000);
		}
	}
	cstr[size] = '\0';
	return cstr;
}

sData_t *sData_open(stream_t *stream)
{
	if( stream == NULL )
		return NULL;
	sData_t *sData = (sData_t*)malloc(sizeof(sData_t));
	stream_setSeek(stream, 0);
	sData->file = stream;
	sData->categoryName = NULL;
	sData->categoryDataOffset = 0; // category data start
	sData->optionLineCount = 0;
	sData->optionLine = NULL;
	return sData;
}

void _sData_preloadCategory(sData_t *sData)
{
	// release data
	if( sData->optionLine )
	{
		for(sint32 i=0; i<sData->optionLineCount; i++)
		{
			if( sData->optionLine[i].optionName )
				free(sData->optionLine[i].optionName);
			if( sData->optionLine[i].optionData )
				free(sData->optionLine[i].optionData);
		}
		free(sData->optionLine);
	}
	// count lines
	stream_setSeek(sData->file, sData->categoryDataOffset);
	char *line = streamEx_readLine(sData->file);
	sint32 lineCount = 0;
	while( line )
	{
		// todo: trim left
		if( line[0] == '[' )
		{
			break;
		}
		char *x = line;
		// skip whitespaces
		while( *x )
		{
			if( *x != sData_whitespaceList[0] && *x != sData_whitespaceList[1] )
				break;
			x++;
		}
		// data lines must start with a letter
		if( (*x >= 'a' && *x <= 'z') || (*x >= 'A' && *x <= 'Z') )
			lineCount++;
		// free line and get next
		free(line);
		line = streamEx_readLine(sData->file);
	}
	// allocate space for lines
	sData->optionLineCount = lineCount;
	if( lineCount == 0 )
	{
		sData->optionLine = NULL;
		return;
	}
	sData->optionLine = (sData_optionLine_t*)malloc(sizeof(sData_optionLine_t) * lineCount);
	// read all data lines
	lineCount = 0;
	stream_setSeek(sData->file, sData->categoryDataOffset);
	line = streamEx_readLine(sData->file);
	lineCount = 0;
	while( line )
	{
		// todo: trim left
		if( line[0] == '[' )
		{
			break;
		}
		char *x = line;
		// skip whitespaces
		while( *x )
		{
			if( *x != sData_whitespaceList[0] && *x != sData_whitespaceList[1] )
				break;
			x++;
		}
		// data lines must start with a letter
		if( (*x >= 'a' && *x <= 'z') || (*x >= 'A' && *x <= 'Z') )
		{
			sint32 splitIdx = -1;
			sint32 tLen = strlen(x);
			// find '='
			for(sint32 i=0; i<tLen; i++)
			{
				if( x[i] == '=' )
				{
					splitIdx = i;
					break;
				}
			}
			if( splitIdx == -1 )
			{
				// only name set...
				// cover with empty data string
				sData->optionLine[lineCount].optionName = (char*)malloc(tLen+1);
				for(sint32 p=0; p<tLen; p++)
					sData->optionLine[lineCount].optionName[p] = x[p];
				sData->optionLine[lineCount].optionName[tLen] = '\0';
				sData->optionLine[lineCount].optionData = (char*)malloc(1);
				sData->optionLine[lineCount].optionData[0] = '\0';
			}
			else
			{
				// name
				sData->optionLine[lineCount].optionName = (char*)malloc(splitIdx+1);
				for(sint32 p=0; p<splitIdx; p++)
					sData->optionLine[lineCount].optionName[p] = x[p];
				sData->optionLine[lineCount].optionName[splitIdx] = '\0';
				// skip '='
				splitIdx++;
				// data - but skip whitespaces first
				sint32 whiteSpaceCount = 0;
				for(sint32 i=0; i<tLen-splitIdx; i++)
				{
					if( x[i+splitIdx] != ' ' && x[i+splitIdx] != '\t' )
						break;
					whiteSpaceCount++;
				}
				sData->optionLine[lineCount].optionData = (char*)malloc(tLen-whiteSpaceCount-splitIdx+1);
				for(sint32 p=0; p<(tLen-splitIdx-whiteSpaceCount); p++)
					sData->optionLine[lineCount].optionData[p] = x[p+splitIdx+whiteSpaceCount];
				sData->optionLine[lineCount].optionData[tLen-splitIdx-whiteSpaceCount] = '\0';
				// cut whitespaces from the end
				sint32 trimIdx = (tLen-splitIdx-whiteSpaceCount);
				trimIdx--;
				while( trimIdx >= 0)
				{
					if( sData->optionLine[lineCount].optionData[trimIdx] == ' ' || sData->optionLine[lineCount].optionData[trimIdx] == '\t' )
						sData->optionLine[lineCount].optionData[trimIdx] = '\0';
					if( sData->optionLine[lineCount].optionData[trimIdx] == '\"' )
						break;
					trimIdx--;
				}
				// filter out "..." strings
				if( sData->optionLine[lineCount].optionData[0] == '\"' )
				{
					uint32 end = 1;
					while( sData->optionLine[lineCount].optionData[end] )
					{	
						if( sData->optionLine[lineCount].optionData[end] == '\"' )
						{
							end--;
							break;
						}
						end++;
					}
					for(uint32 p=0; p<end; p++)
					{
						sData->optionLine[lineCount].optionData[p] = sData->optionLine[lineCount].optionData[p+1];
					}
					sData->optionLine[lineCount].optionData[end] = '\0';
				}
			}
			// cut whitespaces from the name
			sint32 nameLen = strlen(sData->optionLine[lineCount].optionName);
			for(sint32 i=nameLen-1; i>0; i--)
			{
				if( sData->optionLine[lineCount].optionName[i] != sData_whitespaceList[0] && sData->optionLine[lineCount].optionName[i] != sData_whitespaceList[1] )
					break;
				sData->optionLine[lineCount].optionName[i] = '\0';
			}

			lineCount++;
		}
		// free line and get next
		free(line);
		line = streamEx_readLine(sData->file);
	}
}

bool sData_nextCategory(sData_t *sData)
{
	if( sData->categoryName )
	{
		free(sData->categoryName);
		sData->categoryName = NULL;
	}
	stream_setSeek(sData->file, sData->categoryDataOffset);
	char *line = streamEx_readLine(sData->file);
	while( line )
	{
		// todo: trim left
		if( line[0] == '[' )
		{
			// category beginns
			sData->categoryDataOffset = stream_getSeek(sData->file);
			sint32 catLen = min(1024, strlen(line)+1);
			sData->categoryName = (char*)malloc(catLen);
			// copy name
			for(sint32 i=0; i<1023; i++) // 1kb is the name length limit
			{
				sData->categoryName[i] = line[i+1];
				if( sData->categoryName[i] == ']' )
				{
					sData->categoryName[i] = '\0';
					break;
				}
				if( line[i+1] == '\0' )
					break;
			}
			sData->categoryName[catLen-1] = '\0';
			free(line);
			_sData_preloadCategory(sData);
			return true;
		}
		// free line and get next
		free(line);
		line = streamEx_readLine(sData->file);
	}

	return false;
}

char *sData_currentCategoryName(sData_t *sData)
{
	return sData->categoryName;
}

char *sData_findOption(sData_t *sData, char *optionName)
{
	sint32 len = strlen(optionName);
	for(sint32 i=0; i<sData->optionLineCount; i++)
	{
		bool fit = true;
		sint32 oLen = strlen(sData->optionLine[i].optionName);
		if( oLen != len )
			continue;
		for(sint32 l=0; l<len; l++)
		{
			char c1 = sData->optionLine[i].optionName[l];
			char c2 = optionName[l];
			// convert to upper case
			if( c1 >= 'a' && c1 <= 'z' )
				c1 += ('A'-'a');
			if( c2 >= 'a' && c2 <= 'z' )
				c2 += ('A'-'a');
			if( c1 != c2 )
			{
				fit = false;
				break;
			}
		}
		if( fit )
			return sData->optionLine[i].optionData;
	}
	return NULL;
}

void sData_close(sData_t *sData)
{
	if( sData->categoryName )
	{
		free(sData->categoryName);
		sData->categoryName = NULL;
	}
	if( sData->optionLine )
	{
		for(sint32 i=0; i<sData->optionLineCount; i++)
		{
			if( sData->optionLine[i].optionName )
				free(sData->optionLine[i].optionName);
			if( sData->optionLine[i].optionData )
				free(sData->optionLine[i].optionData);
		}
		free(sData->optionLine);
	}
	stream_destroy(sData->file);
	free(sData);
}