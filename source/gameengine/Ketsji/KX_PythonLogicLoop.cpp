#include "KX_KetsjiEngine.h"
#include "KX_KetsjiLogicLoop.h"
#include "PHY_IPhysicsEnvironment.h"
#include "KX_PyConstraintBinding.h"
#include "KX_Scene.h"
#include "KX_PythonLogicLoop.h"
#include "KX_ISceneConverter.h"
#include "SCA_IInputDevice.h"
#include "PyObjectPlus.h"

PyTypeObject KX_PythonLogicLoop::Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"KX_PythonLogicLoop",
	sizeof(PyObjectPlus_Proxy),
	0,
	py_base_dealloc,
	0,
	0,
	0,
	0,
	py_base_repr,
	0,0,0,0,0,0,0,0,0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	0,0,0,0,0,0,0,
	Methods,
	0,
	0,
	&PyObjectPlus::Type,
	0,0,0,0,0,0,
	py_base_new
};

KX_PythonLogicLoop::KX_PythonLogicLoop(KX_KetsjiEngine* engine): KX_KetsjiLogicLoop(engine), PyObjectPlus()
{
}

#ifdef WITH_PYTHON

// Define the method accessors
PyMethodDef KX_PythonLogicLoop::Methods[] = 
{
	{"main", (PyCFunction)	KX_PythonLogicLoop::sPyMain, METH_NOARGS},
	{"start_profile", (PyCFunction)	KX_PythonLogicLoop::sPyStartProfile, METH_VARARGS},
	{"stop_profile", (PyCFunction)	KX_PythonLogicLoop::sPyStopProfile, METH_VARARGS},
	{"update_render", (PyCFunction)	KX_PythonLogicLoop::sPyUpdateRender, METH_NOARGS},
	{"update_blender", (PyCFunction)	KX_PythonLogicLoop::sPyUpdateBlender, METH_NOARGS},
	{"update_messages", (PyCFunction)	KX_PythonLogicLoop::sPyUpdateMessages, METH_NOARGS},
	{"update_keyboard", (PyCFunction)	KX_PythonLogicLoop::sPyUpdateKeyboard, METH_NOARGS},
	{"update_mouse", (PyCFunction)	KX_PythonLogicLoop::sPyUpdateMouse, METH_NOARGS},
	{"update_logic_bricks", (PyCFunction)	KX_PythonLogicLoop::sPyUpdateLogicBricks, METH_O},
	{"update_scenes", (PyCFunction)	KX_PythonLogicLoop::sPyUpdateMouse, METH_NOARGS},
	{"update_physics", (PyCFunction)	KX_PythonLogicLoop::sPyUpdatePhysics, METH_VARARGS},
	{"receive_messages", (PyCFunction)	KX_PythonLogicLoop::sPyReceiveMessages, METH_O},
	{"update_scenegraph", (PyCFunction)	KX_PythonLogicLoop::sPyUpdateScenegraph, METH_O},
	{"update_animations", (PyCFunction)	KX_PythonLogicLoop::sPyUpdateAnimations, METH_O},
	{"set_current_scene", (PyCFunction)	KX_PythonLogicLoop::sPySetCurrentScene, METH_O},
	{"check_quit", (PyCFunction)	KX_PythonLogicLoop::sPyCheckQuit, METH_NOARGS},
	{"get_time", (PyCFunction)	KX_PythonLogicLoop::sPyGetTime, METH_NOARGS},
	{NULL, NULL} //Sentinel
};

// Define the attribute accessors
PyAttributeDef KX_PythonLogicLoop::Attributes[] = {
	{NULL} //Sentinel
};

// Default gameloop main method
PyObject *KX_PythonLogicLoop::PyMain()
{
	// By default this is not implemented
	PyErr_SetString(PyExc_NotImplementedError, "");
	Py_RETURN_NONE;
}

// Start profiling a category, shortcut to KX_KetsjiEngine profiler
PyObject *KX_PythonLogicLoop::PyStartProfile(PyObject *args)
{
	int profile_id;
	bool stop_others = true;
	double current_time = NULL;

	if (PyArg_ParseTuple(args, "i|db:start_profile", &profile_id, &current_time, &stop_others))
	{
		this->StartProfile(profile_id, current_time, stop_others);
		Py_RETURN_NONE;
	}
	return NULL;
}

// Stop profiling a category
PyObject *KX_PythonLogicLoop::PyStopProfile(PyObject *args)
{
	int profile_id;
	double current_time = NULL;

	if (PyArg_ParseTuple(args, "i|d:stop_profile", &profile_id, &current_time))
	{
		this->StopProfile(profile_id, current_time);
		Py_RETURN_NONE;
	}
	return NULL;
}

// Set the current scene that is used for the Python API and collisions
PyObject *KX_PythonLogicLoop::PySetCurrentScene(PyObject *scene)
{
	PyObject *name = PyObject_GetAttrString(scene, "name");
	STR_String scenename = (STR_String)_PyUnicode_AsString(name);
	Py_DECREF(name);

	KX_Scene *gamescene = m_engine->FindScene(scenename);

	if (!gamescene->IsSuspended())
		SetCurrentScene(gamescene);

	Py_RETURN_NONE;
}

// Updates scenes that were added, destroyed or replaced
PyObject *KX_PythonLogicLoop::PyUpdateScenes()
{
	UpdateScenes();
	Py_RETURN_NONE;
}

// Updates logic brick systems
PyObject *KX_PythonLogicLoop::PyUpdateLogicBricks(PyObject *currenttime)
{
	UpdateLogicBricks(PyFloat_AsDouble(currenttime));
	Py_RETURN_NONE;
}

// Updates scenegraph
PyObject *KX_PythonLogicLoop::PyUpdateScenegraph(PyObject *currenttime)
{
	UpdateScenegraph(PyFloat_AsDouble(currenttime));
	Py_RETURN_NONE;
}

// Updates animations
PyObject *KX_PythonLogicLoop::PyUpdateAnimations(PyObject *currenttime)
{
	UpdateAnimations(PyFloat_AsDouble(currenttime));
	Py_RETURN_NONE;
}

// Receives messages from loopback
PyObject *KX_PythonLogicLoop::PyReceiveMessages(PyObject *currenttime)
{
	ReceiveMessages(PyFloat_AsDouble(currenttime));
	Py_RETURN_NONE;
}

// Updates keyboard after frame
PyObject *KX_PythonLogicLoop::PyUpdateKeyboard()
{
	UpdateKeyboard();
	Py_RETURN_NONE;
}

// Updates messages after frame
PyObject *KX_PythonLogicLoop::PyUpdateMessages()
{
	UpdateMessages();
	Py_RETURN_NONE;
}

// Updates mouse after frame
PyObject *KX_PythonLogicLoop::PyUpdateMouse()
{
	UpdateMouse();
	Py_RETURN_NONE;
}

// Updates keyboard and mouse events
PyObject *KX_PythonLogicLoop::PyUpdateBlender()
{
	UpdateBlender();
	Py_RETURN_NONE;
}

// Updates render
PyObject *KX_PythonLogicLoop::PyUpdateRender()
{
	UpdateRender();
	Py_RETURN_NONE;
}

// Returns the elapsed time
PyObject *KX_PythonLogicLoop::PyGetTime()
{
	double time = GetTime();
	PyObject *pytime = PyFloat_FromDouble(time);
	return pytime;
}

// Returns quit status
PyObject *KX_PythonLogicLoop::PyCheckQuit()
{
	bool exitstatus = m_engine->GetExitCode();
 	return PyBool_FromLong(exitstatus);
}

// Updates physics
PyObject *KX_PythonLogicLoop::PyUpdatePhysics(PyObject *args)
{
	double current_time, deltatime;

	if (PyArg_ParseTuple(args, "dd:update_physics",  &current_time, &deltatime))
	{
		this->UpdatePhysics(current_time, deltatime);
		Py_RETURN_NONE;
	}
	return NULL;
}

// Set the python class
void KX_PythonLogicLoop::setPythonClass(PyTypeObject* cls)
{
    PyObject* args = Py_BuildValue("(O)", GetProxy());
    py_base_new(cls, args, NULL);
    Py_DECREF(args);
}		

// Runs the "gameloop", typically a while polling the quit status
void KX_PythonLogicLoop::GiveHandle()
{
	PyObject *self = GetProxy();	
	PyObject *result = PyObject_CallMethod(self, (char*)"main", NULL);

	if (result==NULL)
	{
		PyErr_Print();
	}	
}
#endif

