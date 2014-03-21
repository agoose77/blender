#ifndef __KX_PYTHONLOGICLOOP_H__
#define __KX_PYTHONLOGICLOOP_H__

#include "KX_KetsjiLogicLoop.h"
#include "KX_KetsjiEngine.h"
#include "PyObjectPlus.h"

class KX_PythonLogicLoop : public KX_KetsjiLogicLoop, PyObjectPlus
{
	Py_Header

public:
	KX_PythonLogicLoop(KX_KetsjiEngine *engine);

#ifdef WITH_PYTHON
	//Define a Python function with the name body of "Main"
	KX_PYMETHOD_NOARGS(KX_PythonLogicLoop, Main);
	KX_PYMETHOD_NOARGS(KX_PythonLogicLoop, UpdateBlender);
	KX_PYMETHOD_NOARGS(KX_PythonLogicLoop, UpdateMessages);
	KX_PYMETHOD_NOARGS(KX_PythonLogicLoop, UpdateKeyboard);
	KX_PYMETHOD_NOARGS(KX_PythonLogicLoop, UpdateMouse);
	KX_PYMETHOD_NOARGS(KX_PythonLogicLoop, GetTime);
	KX_PYMETHOD_NOARGS(KX_PythonLogicLoop, CheckQuit);
	KX_PYMETHOD_NOARGS(KX_PythonLogicLoop, UpdateRender);
	KX_PYMETHOD_NOARGS(KX_PythonLogicLoop, UpdateScenes);
	KX_PYMETHOD_O(KX_PythonLogicLoop, UpdateLogicBricks);
	KX_PYMETHOD_O(KX_PythonLogicLoop, UpdateScenegraph);
	KX_PYMETHOD_O(KX_PythonLogicLoop, UpdateAnimations);
	KX_PYMETHOD_O(KX_PythonLogicLoop, SetCurrentScene);
	KX_PYMETHOD_O(KX_PythonLogicLoop, ReceiveMessages);
	KX_PYMETHOD_VARARGS(KX_PythonLogicLoop, UpdatePhysics);
	KX_PYMETHOD_VARARGS(KX_PythonLogicLoop, StartProfile);
	KX_PYMETHOD_VARARGS(KX_PythonLogicLoop, StopProfile);

	void GiveHandle();
	void setPythonClass(PyTypeObject* cls);

#endif

};

#endif