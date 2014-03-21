#ifndef __KX_ENGINECALLBACKDATA_H__
#define __KX_ENGINECALLBACKDATA_H__

struct KX_EngineCallbackData {

	bool (*rendercallback)(KX_EngineCallbackData *data);
	void (*eventcallback)(KX_EngineCallbackData *data);
	void *data;
};
#endif