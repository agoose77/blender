#ifndef __KX_SCA_LOGICLOOP_H__
#define __KX_SCA_LOGICLOOP_H__

#include "KX_KetsjiLogicLoop.h"
#include "KX_KetsjiEngine.h"

class KX_SCA_LogicLoop : public KX_KetsjiLogicLoop
{
public:
	KX_SCA_LogicLoop(KX_KetsjiEngine *engine);
	void GiveHandle();
	void NextFrame();
};

#endif