#ifndef __KX_KETSJILOGICBRICKLOOP_H__
#define __KX_KETSJILOGICBRICKLOOP_H__

#include "KX_KetsjiLogicLoop.h"
#include "KX_KetsjiEngine.h"

class KX_KetsjiLogicBrickLoop : public KX_KetsjiLogicLoop
{
public:
	KX_KetsjiLogicBrickLoop(KX_KetsjiEngine *engine);
	void GiveHandle();
	void NextFrame();
};

#endif