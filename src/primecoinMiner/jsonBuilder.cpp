#include"global.h"

/*
 * Converts an json object and all related subobjects into string format
 */
void jsonBuilder_buildObjectString(fStr_t* fStr_output, jsonObject_t* jsonObject)
{
	if( jsonObject == NULL )
		return; // no json object, no string (this is not necessary an error if parameters are optional)
	if( jsonObject->type == JSON_TYPE_BOOL )
	{
		jsonObjectBool_t* jsonObjectBool = (jsonObjectBool_t*)jsonObject;
		if( jsonObjectBool->isTrue )
			fStr_append(fStr_output, "true");
		else
			fStr_append(fStr_output, "false");
		return;
	}
	
	__debugbreak();
}