#include"JHlib.h"

typedef struct  
{
	CRITICAL_SECTION criticalSection;
	hashTable_t ht_msgQueues;
	uint32 uniqueNameCounter;
}messageQueueEnvironment_t;

messageQueueEnvironment_t messageQueueEnvironment;


/*
 * Initializes the global message queue environment
 * Call this before any other msgQueue_.. function
 */
void msgQueue_init()
{
	InitializeCriticalSection(&messageQueueEnvironment.criticalSection);
	hashTable_init(&messageQueueEnvironment.ht_msgQueues, 4);
	messageQueueEnvironment.uniqueNameCounter = 0x70000000;
}

/*
 * Creates an unconfigured and non-active message queue.
 */
msgQueue_t* msgQueue_create(sint32 nameId, void (JHCALLBACK *messageProc)(msgQueue_t *msgQueue, sint32 msgId, uint32 param1, uint32 param2, void* data))
{
	msgQueue_t *msgQueue = (msgQueue_t*)malloc(sizeof(msgQueue_t));
	RtlZeroMemory(msgQueue, sizeof(msgQueue_t));
	// setup
	InitializeCriticalSection(&msgQueue->criticalSection);
	msgQueue->first = NULL;
	msgQueue->last = NULL;
	msgQueue->nameId = nameId;
	msgQueue->messageProc = messageProc;
	// auto-activate
	EnterCriticalSection(&messageQueueEnvironment.criticalSection);
	hashTable_set(&messageQueueEnvironment.ht_msgQueues, msgQueue->nameId, msgQueue);
	LeaveCriticalSection(&messageQueueEnvironment.criticalSection);
	// return queue
	return msgQueue;
}


/*
 * Generates a unique nameId
 * Generated nameIds are always negative
 * Returns 0 on error
 */
sint32 msgQueue_generateUniqueNameId()
{
	//EnterCriticalSection(&messageQueueEnvironment.criticalSection);
	//for(sint32 i=-1; i>-100000; i--)
	//{
	//	if( hashTable_get(&messageQueueEnvironment.ht_msgQueues, i) == NULL )
	//	{
	//		LeaveCriticalSection(&messageQueueEnvironment.criticalSection);
	//		return i;
	//	}
	//}
	//LeaveCriticalSection(&messageQueueEnvironment.criticalSection);
	//return 0;
	uint32 name = 0;
	EnterCriticalSection(&messageQueueEnvironment.criticalSection);
	name = messageQueueEnvironment.uniqueNameCounter;
	messageQueueEnvironment.uniqueNameCounter++;
	LeaveCriticalSection(&messageQueueEnvironment.criticalSection);
	return name;
}

/*
 * Enables the message queue to receive messages
 */
void msgQueue_activate(msgQueue_t* msgQueue)
{
	EnterCriticalSection(&messageQueueEnvironment.criticalSection);
	hashTable_set(&messageQueueEnvironment.ht_msgQueues, msgQueue->nameId, msgQueue);
	LeaveCriticalSection(&messageQueueEnvironment.criticalSection);
}

/*
 * Should enable the queue to only receive or block certain messages
 * maybe in future..
 */
//void msgQueue_registerFilter(messageQueue_t* msgQueue, sint32 msgId)
//{
//}

//bool msgQueue_check(msgQueue_t* msgQueue, msgInfo_t *msg)
//{
//	if( msg == NULL )
//		return false;
//	EnterCriticalSection(&msgQueue->criticalSection);
//	if( msgQueue->first )
//	{
//		msgInfoLink_t *next = msgQueue->first->next;
//		msg->msgId = msgQueue->first->msgInfo.msgId;
//		msg->paramA = msgQueue->first->msgInfo.paramA;
//		msg->paramB = msgQueue->first->msgInfo.paramB;
//		free(msgQueue->first);
//		if( next == NULL )
//		{
//			msgQueue->first = NULL;
//			msgQueue->last = NULL;
//		}
//		else
//		{
//			msgQueue->first = next;
//		}
//		LeaveCriticalSection(&msgQueue->criticalSection);
//		return true;
//	}
//	LeaveCriticalSection(&msgQueue->criticalSection);
//	return false;
//}

bool msgQueue_check(msgQueue_t* msgQueue)
{
	EnterCriticalSection(&msgQueue->criticalSection);
	if( msgQueue->first )
	{
		msgInfoLink_t *next = msgQueue->first->next;
		msgInfo_t msg;
		msg.msgId = msgQueue->first->msgInfo.msgId;
		msg.paramA = msgQueue->first->msgInfo.paramA;
		msg.paramB = msgQueue->first->msgInfo.paramB;
		msg.data = msgQueue->first->msgInfo.data;
		free(msgQueue->first);
		if( next == NULL )
		{
			msgQueue->first = NULL;
			msgQueue->last = NULL;
		}
		else
		{
			msgQueue->first = next;
		}
		msgQueue->messageProc(msgQueue, msg.msgId, msg.paramA, msg.paramB, msg.data);
		if( msg.data ) free(msg.data);
		LeaveCriticalSection(&msgQueue->criticalSection);
		return true;
	}
	LeaveCriticalSection(&msgQueue->criticalSection);
	return false;
}

#define MSGQUEUE_ALL	0x7FFFFFFF

bool msgQueue_postMessage(sint32 destNameId, sint32 msgId, uint32 param1, uint32 param2, void* data)
{
	if( destNameId == MSGQUEUE_ALL )
	{
		// send to all
	}
	else
	{
		// send to specific
		//EnterCriticalSection(&messageQueueEnvironment.criticalSection);
		msgQueue_t *msgQueue = (msgQueue_t*)hashTable_get(&messageQueueEnvironment.ht_msgQueues, destNameId);
		if( msgQueue == NULL )
		{
			if( data ) free(data);
			return false;
		}
		// allocate and setup message
		msgInfoLink_t *msg = (msgInfoLink_t*)malloc(sizeof(msgInfoLink_t));
		if( msg == NULL )
			return false;
		msg->msgInfo.msgId = msgId;
		msg->msgInfo.paramA = param1;
		msg->msgInfo.paramB = param2;
		msg->msgInfo.data = data;
		// append
		EnterCriticalSection(&msgQueue->criticalSection);
		if( msgQueue->last == NULL )
		{
			if( msgQueue->first != NULL )
				__debugbreak(); //BUG!
			// new entry
			msg->next = NULL;
			msgQueue->first = msg;
			msgQueue->last = msg;
		}
		else
		{
			// append to last
			if( msgQueue->last->next )
				__debugbreak();
			msg->next = NULL;
			msgQueue->last->next = msg;
			msgQueue->last = msg;
		}
		LeaveCriticalSection(&msgQueue->criticalSection);
	}
	return true;
}