#include"global.h"

uint8* json_emptyString = (uint8*)"";

void jsonParser_skipWhitespaces(jsonParser_t* jsonParser)
{
	while( jsonParser->dataCurrentPtr < jsonParser->dataEnd )
	{
		// skip whitespaces
		uint8 c = *jsonParser->dataCurrentPtr;
		if( c == ' ' || c == '\t' || c == '\r' || c == '\n' )
		{
			jsonParser->dataCurrentPtr++;
			continue;
		}
		else
			return;
	}
	return;
}

/*
 * Trys to read a string from the json stream
 * If it fails, returns NULL and the current parser location is left untouched.
 */
uint8* jsonParser_readString(jsonParser_t* jsonParser, uint32 maxLength, uint32* length)
{
	uint8* backupPtr = jsonParser->dataCurrentPtr;
	jsonParser_skipWhitespaces(jsonParser);
	if( jsonParser->dataCurrentPtr+2 > jsonParser->dataEnd )
		return NULL; // a string never can have less than 2 chars (smallest string def: "")
	uint8 c = *jsonParser->dataCurrentPtr;
	if( c != '\"' )
		return NULL;
	jsonParser->dataCurrentPtr++;
	// first try to find length of full string
	uint8* parsePtr = jsonParser->dataCurrentPtr;
	sint32 stringLength = -1;
	while( parsePtr < jsonParser->dataEnd )
	{
		// todo: Correctly process UTF8 characters
		if( *parsePtr == '\"' )
		{
			stringLength = (sint32)(parsePtr - jsonParser->dataCurrentPtr);
			parsePtr++; // also skip the '"'
			break;
		}
		// next char
		parsePtr++;
	}
	if( stringLength == -1 )
	{
		// never ending strings are not allowed ('"' missing)
		jsonParser->dataCurrentPtr = backupPtr;
		return NULL;
	}
	// stringLength is now the effective length of the string in bytes (without the quotation marks)
	// note: if the string is too long we simply copy as many bytes as allowed and then skip the rest
	sint32 effectiveStringLength = min((sint32)maxLength, stringLength);
	uint8* stringBuffer = NULL;
	if( effectiveStringLength == 0 )
		stringBuffer = json_emptyString;
	else
		stringBuffer = (uint8*)malloc(effectiveStringLength);
	// copy string data
	memcpy(stringBuffer, jsonParser->dataCurrentPtr, effectiveStringLength);
	jsonParser->dataCurrentPtr = parsePtr;
	*length = effectiveStringLength;
	return stringBuffer;
}

jsonObject_t* jsonParser_parseObject(jsonParser_t* jsonParser)
{
	// init json base object
	//jsonObject_t* jsonObject = (jsonObject_t*)malloc(sizeof(jsonObject_t));
	//RtlZeroMemory(jsonObject, sizeof(jsonObject_t));
	//jsonObject->type = 0;
	// parse
	while( jsonParser->dataCurrentPtr < jsonParser->dataEnd )
	{
		// skip whitespaces
		jsonParser_skipWhitespaces(jsonParser);
		uint8 c = *jsonParser->dataCurrentPtr;
		// check for '{'
		if( c == '{' )
		{
			// start of raw object
			jsonObjectRawObject_t* jsonObjectRawObject = (jsonObjectRawObject_t*)malloc(sizeof(jsonObjectRawObject_t));
			RtlZeroMemory(jsonObjectRawObject, sizeof(jsonObjectRawObject_t));
			jsonObjectRawObject->baseObject.type = JSON_TYPE_OBJECT;
			jsonObjectRawObject->list_paramPairs = customBuffer_create(4, sizeof(jsonObjectRawObjectParameter_t));
			jsonParser->dataCurrentPtr++;
			// parse up to 4096 string:value pairs
			for(int p=0; p<4096; p++)
			{
				jsonParser_skipWhitespaces(jsonParser);
				jsonObjectRawObjectParameter_t objectParameter = {0};
				// end of object reached?
				if( *jsonParser->dataCurrentPtr == '}' )
				{
					jsonParser->dataCurrentPtr++;
					return (jsonObject_t*)jsonObjectRawObject;
				}
				// next object delimiter ','
				if( p > 0 && *jsonParser->dataCurrentPtr != ',' )
				{
					printf("JSON: Syntax error at character %d - Missing ',' between object parameters\n", (sint32)(jsonParser->dataCurrentPtr - jsonParser->dataBuffer));
					jsonObject_freeObject((jsonObject_t*)jsonObjectRawObject);
					return NULL;
				}
				if( p > 0 )
				{
					jsonParser->dataCurrentPtr++;
					jsonParser_skipWhitespaces(jsonParser);
				}
				// todo: In case of error free ALL already parsed data
				// read string if possible
				uint32 objectParamNameLength = 0;
				uint8* objectParamName = jsonParser_readString(jsonParser, 4096, &objectParamNameLength); // max param length -> 4096
				if( objectParamName == NULL )
				{
					// failed to correctly parse the required param name string but not at end of object yet
					free(jsonObjectRawObject);
					return NULL;
				}
				objectParameter.stringNameData = objectParamName;
				objectParameter.stringNameLength = objectParamNameLength;
				// read name/value delimiter ':'
				jsonParser_skipWhitespaces(jsonParser);
				if( jsonParser->dataCurrentPtr+1 > jsonParser->dataEnd )
				{
					printf("JSON: Syntax error at character %d\n", (sint32)(jsonParser->dataCurrentPtr - jsonParser->dataBuffer));
					jsonObject_freeStringData(objectParamName);
					jsonObject_freeObject((jsonObject_t*)jsonObjectRawObject);
					return NULL;
				}
				if( *jsonParser->dataCurrentPtr != ':' )
				{
					printf("JSON: Syntax error at character %d - Object Name/Value delimiter \':\' missing\n", (sint32)(jsonParser->dataCurrentPtr - jsonParser->dataBuffer));
					jsonObject_freeStringData(objectParamName);
					jsonObject_freeObject((jsonObject_t*)jsonObjectRawObject);
					return NULL;
				}
				jsonParser->dataCurrentPtr++; // skip the ':'
				// read value
				jsonObject_t* subObject = jsonParser_parseObject(jsonParser);
				if( subObject == NULL )
				{
					// failed to parse value (subobject) -> Error already displayed by the failed call
					jsonObject_freeStringData(objectParamName);
					jsonObject_freeObject((jsonObject_t*)jsonObjectRawObject);
					return NULL;
				}
				objectParameter.jsonObjectValue = subObject;
				// register name/value pair
				customBuffer_add(jsonObjectRawObject->list_paramPairs, &objectParameter, 1);
			}
		}
		else if( c == '}' )
		{
			// end of raw object (should never occur separate)
			printf("JSON: Syntax error at character %d - Found end-of-object but no object start character\n", (sint32)(jsonParser->dataCurrentPtr - jsonParser->dataBuffer));
			return NULL;
		}
		else if( (jsonParser->dataCurrentPtr+4 < jsonParser->dataEnd) &&
			(jsonParser->dataCurrentPtr[0] == 't' || jsonParser->dataCurrentPtr[0] == 'T') &&
			(jsonParser->dataCurrentPtr[1] == 'r' || jsonParser->dataCurrentPtr[1] == 'R') &&
			(jsonParser->dataCurrentPtr[2] == 'u' || jsonParser->dataCurrentPtr[2] == 'U') &&
			(jsonParser->dataCurrentPtr[3] == 'e' || jsonParser->dataCurrentPtr[3] == 'E') )
		{
			jsonObjectBool_t* jsonObjectBool = (jsonObjectBool_t*)malloc(sizeof(jsonObjectBool_t));
			RtlZeroMemory(jsonObjectBool, sizeof(jsonObjectBool_t));
			jsonObjectBool->baseObject.type = JSON_TYPE_BOOL;
			jsonObjectBool->isTrue = true;
			jsonParser->dataCurrentPtr += 4;
			return (jsonObject_t*)jsonObjectBool;
		}
		else if( (jsonParser->dataCurrentPtr+5 < jsonParser->dataEnd) &&
			(jsonParser->dataCurrentPtr[0] == 'f' || jsonParser->dataCurrentPtr[0] == 'f') &&
			(jsonParser->dataCurrentPtr[1] == 'a' || jsonParser->dataCurrentPtr[1] == 'a') &&
			(jsonParser->dataCurrentPtr[2] == 'l' || jsonParser->dataCurrentPtr[2] == 'l') &&
			(jsonParser->dataCurrentPtr[3] == 's' || jsonParser->dataCurrentPtr[3] == 's') &&
			(jsonParser->dataCurrentPtr[4] == 'e' || jsonParser->dataCurrentPtr[4] == 'e') )
		{
			jsonObjectBool_t* jsonObjectBool = (jsonObjectBool_t*)malloc(sizeof(jsonObjectBool_t));
			RtlZeroMemory(jsonObjectBool, sizeof(jsonObjectBool_t));
			jsonObjectBool->baseObject.type = JSON_TYPE_BOOL;
			jsonObjectBool->isTrue = false;
			jsonParser->dataCurrentPtr += 5;
			return (jsonObject_t*)jsonObjectBool;
		}
		else if( c == '\"' )
		{
			uint32 stringLength = 0;
			uint8* stringData = jsonParser_readString(jsonParser, 1024*1024*1, &stringLength);
			if( stringData == NULL )
			{
				return NULL;
			}
			// setup string
			jsonObjectString_t* jsonObjectString = (jsonObjectString_t*)malloc(sizeof(jsonObjectString_t));
			RtlZeroMemory(jsonObjectString, sizeof(jsonObjectString_t));
			jsonObjectString->baseObject.type = JSON_TYPE_STRING;
			jsonObjectString->stringData = stringData;
			jsonObjectString->stringLength = stringLength;
			return (jsonObject_t*)jsonObjectString;
		}
		else if( c == '[' )
		{
			// start of array
			jsonObjectArray_t* jsonObjectArray = (jsonObjectArray_t*)malloc(sizeof(jsonObjectArray_t));
			RtlZeroMemory(jsonObjectArray, sizeof(jsonObjectArray_t));
			jsonObjectArray->baseObject.type = JSON_TYPE_ARRAY;
			jsonObjectArray->list_values = simpleList_create(4);
			jsonParser->dataCurrentPtr++;
			// parse up to 8192 values pairs
			for(int p=0; p<8192; p++)
			{
				jsonParser_skipWhitespaces(jsonParser);
				// end of array reached?
				if( *jsonParser->dataCurrentPtr == ']' )
				{
					jsonParser->dataCurrentPtr++;
					return (jsonObject_t*)jsonObjectArray;
				}
				if( p > 0 )
				{
					if( *jsonParser->dataCurrentPtr == ',' )
					{
						jsonParser->dataCurrentPtr++;
					}
					else
					{
						printf("JSON: Syntax error at character %d - Expected ','\n", (sint32)(jsonParser->dataCurrentPtr - jsonParser->dataBuffer));
						jsonObject_freeObject((jsonObject_t*)jsonObjectArray);
						return NULL;
					}
				}
				// read value
				jsonObject_t* subObject = jsonParser_parseObject(jsonParser);
				if( subObject == NULL )
				{
					// failed to parse value (subobject) -> Error already displayed by the failed call
					jsonObject_freeObject((jsonObject_t*)jsonObjectArray);
					return NULL;
				}
				// register name/value pair
				simpleList_add(jsonObjectArray->list_values, subObject);
			}
		}
		else if( c == '-' || (c >= '0' && c <= '9') )
		{
			// parse number
			sint64 integralPart = 0;
			uint64 divider = 1;
			bool foundDot = false;
			// is negative?
			bool isNegative = false;
			if( c == '-' )
			{
				isNegative = true;
				jsonParser->dataCurrentPtr++;
			}
			// parse digits
			uint32 numOfDigits = 0;
			while( jsonParser->dataCurrentPtr < jsonParser->dataEnd )
			{
				if( jsonParser->dataCurrentPtr[0] == '.' )
				{
					foundDot = true;
					jsonParser->dataCurrentPtr++;
					continue;
				}
				if( jsonParser->dataCurrentPtr[0] < '0' || jsonParser->dataCurrentPtr[0] > '9' )
					break;
				if( foundDot )
					divider *= 10ULL;
				integralPart *= (sint64)10LL;
				integralPart += (sint64)(jsonParser->dataCurrentPtr[0] - '0');
				numOfDigits++;
				// next
				jsonParser->dataCurrentPtr++;
			}
			if( isNegative )
				integralPart = -integralPart;
			if( numOfDigits == 0 )
			{
				// a number without digits is not allowed
				printf("JSON: Syntax error at character %d - Found '-' without digits\n", (sint32)(jsonParser->dataCurrentPtr - jsonParser->dataBuffer));
				return NULL;
			}
			// set number
			jsonObjectNumber_t* jsonObjectNumber = (jsonObjectNumber_t*)malloc(sizeof(jsonObjectNumber_t));
			RtlZeroMemory(jsonObjectNumber, sizeof(jsonObjectNumber_t));
			jsonObjectNumber->baseObject.type = JSON_TYPE_NUMBER;
			jsonObjectNumber->divider = divider;
			jsonObjectNumber->number = integralPart;
			return (jsonObject_t*)jsonObjectNumber;
		}
		else if( (jsonParser->dataCurrentPtr+4 < jsonParser->dataEnd) && jsonParser->dataCurrentPtr[0] == 'n' && jsonParser->dataCurrentPtr[1] == 'u' && jsonParser->dataCurrentPtr[2] == 'l' && jsonParser->dataCurrentPtr[3] == 'l' )
		{
			jsonParser->dataCurrentPtr += 4;
			// for NULL objects we dont need any data stored, so we use the pure jsonObject_t object
			jsonObject_t* jsonObject = (jsonObject_t*)malloc(sizeof(jsonObject_t));
			RtlZeroMemory(jsonObject, sizeof(jsonObject_t));
			jsonObject->type = JSON_TYPE_NULL;
			return (jsonObject_t*)jsonObject;
		}
		else
		{
			return NULL;//__debugbreak(); // todo - error handler --> Invalid object syntax
		}
		// next char
		jsonParser->dataCurrentPtr++;
	}
	return NULL;
}

jsonObject_t* jsonParser_parse(uint8* stream, uint32 dataLength)
{
	// init local parser instance
	jsonParser_t jsonParser;
	jsonParser.dataBuffer = stream;
	jsonParser.dataLength = dataLength;
	jsonParser.dataCurrentPtr = jsonParser.dataBuffer;
	jsonParser.dataEnd = jsonParser.dataBuffer+dataLength;
	// init json base object
	jsonObject_t* jsonObject = jsonParser_parseObject(&jsonParser);
	return jsonObject;
}