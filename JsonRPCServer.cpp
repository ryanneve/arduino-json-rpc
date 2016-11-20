/*
  JsonRPCServer.cpp - Simple JSON-RPC Server for Arduino
  Created by Meir Tseitlin, March 5, 2014. This code is based on https://code.google.com/p/ajson-rpc/
  Modified by frmdstryr, 19 Nov 2016
  Released under GPLv2 license.
*/
#include "Arduino.h"
#include "aJSON.h"
#include "JsonRPCServer.h"

JsonRPCServer::JsonRPCServer(Stream* stream): _jsonStream(stream) {
	
}


void JsonRPCServer::begin(int capacity)
{
    mymap = (FuncMap *)malloc(sizeof(FuncMap));
    mymap->capacity = capacity;
    mymap->used = 0;
    mymap->mappings = (Mapping*)malloc(capacity * sizeof(Mapping));
    memset(mymap->mappings, 0, capacity * sizeof(Mapping));
	
	// Register all json procedures (pure virtual - implemented in derivatives with macros)
	registerProcs();
}

void JsonRPCServer::registerMethod(String methodName, JSON_PROC_STATIC_T callback, JSON_RPC_RET_TYPE type)
{
    // only write keyvalue pair if we allocated enough memory for it
    if (mymap->used < mymap->capacity)
    {
	    Mapping* mapping = &(mymap->mappings[mymap->used++]);
	    mapping->name = methodName;
	    mapping->callback = callback;
		mapping->retType = type;
    }
}


void JsonRPCServer::processMessage(aJsonObject *msg)
{
    aJsonObject* method = aJson.getObjectItem(msg, "method");
    aJsonObject *response = aJson.createObject();
    if (!method)
    {

    	aJsonObject *error = aJson.createObject();
    	// not a valid Json-RPC message
    	aJson.addItemToObject(error, "code", aJson.createItem(-32600));
    	aJson.addItemToObject(error, "message", aJson.createItem("Invalid Request."));
    	aJson.addItemToObject(response,"error",error);
		aJson.print(response, &_jsonStream);
		aJson.deleteItem(error);
		aJson.deleteItem(response);
        return;
    }
    
    aJsonObject* params = aJson.getObjectItem(msg, "params");

    
    String methodName = method->valuestring;
    for (unsigned int i=0; i<mymap->used; i++)
    {
        Mapping* mapping = &(mymap->mappings[i]);
        if (methodName.equals(mapping->name))
        {
			switch (mapping->retType) {
			case	JSON_RPC_RET_TYPE_NONE:
				mapping->callback(this, params);
				aJson.addItemToObject(response, "result", aJson.createNull());
				break;
			case	JSON_RPC_RET_TYPE_INT:
				{
					JSON_PROC_INT_STATIC_T callback = (JSON_PROC_INT_STATIC_T) mapping->callback;
					
					int ret = callback(this, params);
					aJson.addItemToObject(response, "result", aJson.createItem((int)ret));
				}
				break;
			case	JSON_RPC_RET_TYPE_BOOL:
				{
					JSON_PROC_BOOL_STATIC_T callback = (JSON_PROC_BOOL_STATIC_T) mapping->callback;
					
					bool ret = callback(this, params);
					aJson.addItemToObject(response, "result", aJson.createItem(ret));
				}
				break;
			case	JSON_RPC_RET_TYPE_FLOAT:
				{
					JSON_PROC_FLOAT_STATIC_T callback = (JSON_PROC_FLOAT_STATIC_T) mapping->callback;

					float ret = callback(this, params);
					aJson.addItemToObject(response, "result", aJson.createItem(ret));
				}
				break;
			case	JSON_RPC_RET_TYPE_DOUBLE:
				{
					JSON_PROC_DOUBLE_STATIC_T callback = (JSON_PROC_DOUBLE_STATIC_T) mapping->callback;

					double ret = callback(this, params);
					aJson.addItemToObject(response, "result", aJson.createItem(ret));
				}
				break;
			case	JSON_RPC_RET_TYPE_STRING:
				{
					JSON_PROC_STRING_STATIC_T callback = (JSON_PROC_STRING_STATIC_T) mapping->callback;
					String ret = callback(this, params);
					aJson.addItemToObject(response, "result", aJson.createItem(ret.c_str()));
					
				}
				break;
//			case	JSON_RPC_RET_TYPE_OBJECT:
//				{
//					JSON_PROC_OBJECT_T callback = (JSON_PROC_OBJECT_T) mapping->callback;
//					aJsonObject ret = callback(this, params);
//					aJson.addItemToObject(response, "result", ret);
//					aJson.print(response, &_jsonStream);
//					aJson.deleteItem(ret);
//					aJson.deleteItem(response);
//					return;
//
//				}
//				break;
			}
			
			aJson.print(response, &_jsonStream);
			aJson.deleteItem(response);
			return;
		}
    }

    // If we get here... the Method is not found
	aJsonObject *error = aJson.createObject();
	aJson.addItemToObject(error, "code", aJson.createItem(-32601));
	aJson.addItemToObject(error, "message", aJson.createItem("Method not found."));
	aJson.addItemToObject(response,"error",error);
	aJson.print(response, &_jsonStream);
	aJson.deleteItem(error);
	aJson.deleteItem(response);
}

void JsonRPCServer::process() {
	
	/* add main program code here */
	if (_jsonStream.available()) {

		// skip any accidental whitespace like newlines
		_jsonStream.skip();
	}

	if (_jsonStream.available()) {
		
		aJsonObject *msg = aJson.parse(&_jsonStream);
		if (msg) {
			processMessage(msg);
			aJson.deleteItem(msg);
			} else {
				aJsonObject *response = aJson.createObject();
				aJsonObject *error = aJson.createObject();
				aJson.addItemToObject(error, "code", aJson.createItem(-32700));
				aJson.addItemToObject(error, "message", aJson.createItem("Parse error."));
				aJson.addItemToObject(response,"error",error);
				aJson.print(response, &_jsonStream);
				aJson.deleteItem(error);
				aJson.deleteItem(response);
		}
	}
	
}
