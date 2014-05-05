#include "KX_KetsjiSetupTools.h"
#include "KX_PythonLogicLoop.h"
#include "BL_BlenderDataConversion.h"
#include "RAS_OpenGLRasterizer.h"
#include "RAS_ICanvas.h"
#include "Value.h"

void setupPythonGameloop(KX_KetsjiEngine *ketsjiengine, char *custom_loop)
{
	// Setup python gameloop

	if (!strcmp(custom_loop, ""))
	{
		return;
	}
	// Collect the path information
	std::vector<STR_String> path_parts = STR_String(custom_loop).Explode('.');

		if (path_parts.size() < 2)
	{
			printf("Python class name formatting error:\n\texpected \"SomeModule.Class\", got \"%s\"\n", custom_loop);
			return;
	}

	// Now recombine the module path (excludes the class)
	STR_String mod_path = STR_String("");
	char* class_string = path_parts[path_parts.size()-1].Ptr();
	int parts_size = path_parts.size();
	int last_index = parts_size - 1;
	int j;

	for (j = 0; j < last_index; j++)
	{
		mod_path += path_parts[j];
		if (j != last_index - 1)
			mod_path += ".";
	}

	// Import the module containing the path
	PyObject* mod = PyImport_ImportModule(mod_path.Ptr());

	if (mod == NULL)
	{
			printf("Python module not found: %s\n", mod_path.Ptr());
			PyErr_Print();
			return;
	}

	// Get the class object
	PyObject* py_class = PyObject_GetAttrString(mod, class_string);

	if (py_class == NULL)
	{
			printf("Python module found, but could not find class \"%s\"\n", class_string);
			Py_DECREF(mod);
			return;
	}

	// Check to make sure the object is actually a class
	if (!PyType_Check(py_class))
	{
			printf("%s is not a class\n", class_string);
			Py_DECREF(mod);
			Py_DECREF(py_class);
			return;
	}

	// Finally set the class
	KX_PythonLogicLoop *loop = new KX_PythonLogicLoop(ketsjiengine);
	loop->setPythonClass((PyTypeObject*) py_class);

	// Tell engine to setup using loop (old one is deleted)
	ketsjiengine->SetLogicLoop(loop);

	Py_DECREF(mod);
	Py_DECREF(py_class);
}

KX_KetsjiEngine *setupKetsjiEngine(Scene *startscene, RAS_ICanvas *canvas, GlobalSettings *globalsettings, 
								   SCA_IInputDevice *keyboard, SCA_IInputDevice *mouse, NG_NetworkDeviceInterface *network,
								   KX_ISystem *system)
{
		SYS_SystemHandle syshandle = SYS_GetSystem();
		bool properties	= (SYS_GetCommandLineInt(syshandle, "show_properties", 0) != 0);
		bool profile = (SYS_GetCommandLineInt(syshandle, "show_profile", 0) != 0);
		bool frame_rate = (SYS_GetCommandLineInt(syshandle, "show_framerate", 0) != 0);
		bool animation_record = (SYS_GetCommandLineInt(syshandle, "animation_record", 0) != 0);
		bool use_fixed_timestep = (SYS_GetCommandLineInt(syshandle, "fixedtime", 0) != 0) && !animation_record;
		bool use_display_lists = (SYS_GetCommandLineInt(syshandle, "displaylists", 0) != 0) && GPU_display_list_support();
#ifdef WITH_PYTHON
		bool show_deprecation_warnings = (SYS_GetCommandLineInt(syshandle, "ignore_deprecation_warnings", 0) != 0);
#endif
		// bool novertexarrays = (SYS_GetCommandLineInt(syshandle, "novertexarrays", 0) != 0);
		bool mouse_state = startscene->gm.flag & GAME_SHOW_MOUSE;
		bool use_animation_frame_rate = startscene->gm.flag & GAME_RESTRICT_ANIM_UPDATES;

		// Determine user-specified mouse mode
		RAS_ICanvas::RAS_MouseState mouse_mode = mouse_state ? RAS_ICanvas::MOUSE_NORMAL : RAS_ICanvas::MOUSE_INVISIBLE;
		// Apply to canvas
		canvas->SetMouseState(mouse_mode);

		int vsync_value = (startscene->gm.vsync == VSYNC_ADAPTIVE) ? -1 : startscene->gm.vsync;
		canvas->SetSwapInterval(vsync_value); // VSYNC_OFF == 0, VSYNC_ON == 1, so this works

		RAS_IRasterizer* rasterizer = NULL;
		//Don't use use_display_lists with VBOs
		//If auto starts using VBOs, make sure to check for that here
		if (use_display_lists && startscene->gm.raster_storage != RAS_STORE_VBO)
			rasterizer = new RAS_ListRasterizer(canvas, true, startscene->gm.raster_storage);
		else
			rasterizer = new RAS_OpenGLRasterizer(canvas, startscene->gm.raster_storage);
		
		// create the ketsjiengine
		KX_KetsjiEngine* ketsjiengine = new KX_KetsjiEngine(system);
		
		// set the devices
		ketsjiengine->SetKeyboardDevice(keyboard);
		ketsjiengine->SetMouseDevice(mouse);
		ketsjiengine->SetNetworkDevice(network);
		ketsjiengine->SetCanvas(canvas);
		ketsjiengine->SetRasterizer(rasterizer);

		// set the local settings
		ketsjiengine->SetUseFixedTime(use_fixed_timestep);
		ketsjiengine->SetAnimRecordMode(animation_record);
		ketsjiengine->SetTimingDisplay(frame_rate, profile, properties);
		ketsjiengine->SetRestrictAnimationFPS(use_animation_frame_rate);

		// set the exit key
		KX_KetsjiEngine::SetExitKey(ConvertKeyCode(startscene->gm.exitkey));

	// set the global settings (carried over if restart/load new files)
	ketsjiengine->SetGlobalSettings(globalsettings);

	// set the warning status for deprecated methods/accessors
#ifdef WITH_PYTHON
		CValue::SetDeprecationWarnings(show_deprecation_warnings);
#endif

	return ketsjiengine;
};