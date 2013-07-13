#include"global.h"

/*
 * Tries to find the parameter defined by the case-insenitive name.
 * jsonObject must be of type raw object
 */
jsonObject_t* jsonObject_getParameter(jsonObject_t* jsonObject, char* name)
{
	if( jsonObject->type != JSON_TYPE_OBJECT )
		return NULL;
	jsonObjectRawObject_t* jsonObjectRawObject = (jsonObjectRawObject_t*)jsonObject;
	sint32 strLen = fStrLen(name);
	for(uint32 i=0; i<jsonObjectRawObject->list_paramPairs->objectCount; i++)
	{
		jsonObjectRawObjectParameter_t* objParam = (jsonObjectRawObjectParameter_t*)customBuffer_get(jsonObjectRawObject->list_paramPairs, i);
		if( objParam->stringNameLength != strLen || fStrCmpCaseInsensitive((uint8*)name, objParam->stringNameData, strLen) )
			continue;
		return objParam->jsonObjectValue;
	}
	return NULL;
}

/*
 * Gets the size of the array in elements
 */
uint32 jsonObject_getArraySize(jsonObject_t* jsonObject)
{
	if( jsonObject->type != JSON_TYPE_ARRAY )
		return NULL;
	return ((jsonObjectArray_t*)jsonObject)->list_values->objectCount;
}

/*
 * Returns the n-th element of an json array
 */
jsonObject_t* jsonObject_getArrayElement(jsonObject_t* jsonObject, uint32 index)
{
	if( jsonObject->type != JSON_TYPE_ARRAY )
		return NULL;
	jsonObjectArray_t* jsonObjectArray = (jsonObjectArray_t*)jsonObject;
	if( index >= jsonObjectArray->list_values->objectCount )
		return NULL;
	return (jsonObject_t*)jsonObjectArray->list_values->objects[index];
}

/*
 * Returns the type of the json object
 */
uint32 jsonObject_getType(jsonObject_t* jsonObject)
{
	return jsonObject->type;
}

/*
 * Returns true if the passed object is a bool and has the value true
 * If the object is not a bool or is a bool with the value 'false', false is returned
 */
bool jsonObject_isTrue(jsonObject_t* jsonObject)
{
	if( jsonObject->type != JSON_TYPE_BOOL )
		return false;
	return ((jsonObjectBool_t*)jsonObject)->isTrue;
}

/*
 * Returns the value of the number as a double
 */
double jsonObject_getNumberValueAsDouble(jsonObject_t* jsonObject)
{
	if( jsonObject->type != JSON_TYPE_NUMBER )
		return 0.0;
	jsonObjectNumber_t* jsonObjectNumber = (jsonObjectNumber_t*)jsonObject;
	double v = (double)jsonObjectNumber->number / (double)jsonObjectNumber->divider;
	return v;
}

/*
 * Returns the value of the number as a signed integer
 */
sint32 jsonObject_getNumberValueAsS32(jsonObject_t* jsonObject)
{
	if( jsonObject->type != JSON_TYPE_NUMBER )
		return 0;
	jsonObjectNumber_t* jsonObjectNumber = (jsonObjectNumber_t*)jsonObject;
	sint64 v = (sint64)jsonObjectNumber->number / (sint64)jsonObjectNumber->divider;
	return (sint32)v;
}


/*
 * Returns string info if the given jsonObject is of type string
 * Returns a valid pointer even if the string has a length of zero
 */
uint8* jsonObject_getStringData(jsonObject_t* jsonObject, uint32* length)
{
	if( jsonObject->type != JSON_TYPE_STRING )
		return NULL;
	jsonObjectString_t* jsonObjectString = (jsonObjectString_t*)jsonObject;
	*length = jsonObjectString->stringLength;
	return jsonObjectString->stringData;
}

/*
 * Frees the string data allocated by _readString()
 */
void jsonObject_freeStringData(uint8* stringBuffer)
{
	if( stringBuffer == NULL )
		return;
	if( stringBuffer != json_emptyString )
		free(stringBuffer);
}


/*
 * Helper method in case parsing of an raw object fails and the already parsed data needs to be freed again
 */
void jsonObject_destroyRawObject(jsonObjectRawObject_t* jsonObjectRawObject)
{
	for(uint32 i=0; i<jsonObjectRawObject->list_paramPairs->objectCount; i++)
	{
		jsonObjectRawObjectParameter_t* objParam = (jsonObjectRawObjectParameter_t*)customBuffer_get(jsonObjectRawObject->list_paramPairs, i);
		jsonObject_freeStringData(objParam->stringNameData);
		jsonObject_freeObject(objParam->jsonObjectValue);
	}
	customBuffer_free(jsonObjectRawObject->list_paramPairs);
	free(jsonObjectRawObject);
}

/*
 * Helper method in case parsing of an array fails and the already parsed data needs to be freed again
 */
void jsonObject_destroyArray(jsonObjectArray_t* jsonObjectArray)
{
	for(uint32 i=0; i<jsonObjectArray->list_values->objectCount; i++)
	{
		jsonObject_t* objValue = (jsonObject_t*)simpleList_get(jsonObjectArray->list_values, i);
		jsonObject_freeObject(objValue);
	}
	simpleList_free(jsonObjectArray->list_values);
	free(jsonObjectArray);
}


/*
 * Called to free a json object and all subobjects
 */
void jsonObject_freeObject(jsonObject_t* jsonObject)
{
	if( jsonObject->type == JSON_TYPE_OBJECT )
		jsonObject_destroyRawObject((jsonObjectRawObject_t*)jsonObject);
	else if( jsonObject->type == JSON_TYPE_ARRAY )
		jsonObject_destroyArray((jsonObjectArray_t*)jsonObject);
	else if( jsonObject->type == JSON_TYPE_STRING )
	{
		jsonObjectString_t* jsonString = (jsonObjectString_t*)jsonObject;
		jsonObject_freeStringData(jsonString->stringData);
		free(jsonString);
	}
	else if( jsonObject->type == JSON_TYPE_NUMBER )
	{
		free(jsonObject);
	}
	else if( jsonObject->type == JSON_TYPE_NULL )
	{
		free(jsonObject);
	}
	else if( jsonObject->type == JSON_TYPE_BOOL )
	{
		free(jsonObject);
	}
	else
		__debugbreak();
}