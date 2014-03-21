#ifndef __KX_KETSJISETUPTOOLS_H__
#define __KX_KETSJISETUPTOOLS_H__
#include "KX_KetsjiEngine.h"
#include "BL_System.h"
#include "GPU_extensions.h"

#include "RAS_GLExtensionManager.h"
#include "RAS_OpenGLRasterizer.h"
#include "RAS_ListRasterizer.h"
#include "KX_BlenderSceneConverter.h"

#include "SCA_IInputDevice.h"

extern "C" {
	#include "DNA_view3d_types.h"
	#include "DNA_screen_types.h"
	#include "DNA_userdef_types.h"
	#include "DNA_scene_types.h"
	#include "DNA_windowmanager_types.h"

	#include "BKE_global.h"
	#include "BKE_report.h"
	#include "BKE_ipo.h"
	#include "BKE_main.h"
	#include "BKE_context.h"

	/* avoid c++ conflict with 'new' */
	#define new _new
	#include "BKE_screen.h"
	#undef new

	#include "MEM_guardedalloc.h"

	#include "BLI_blenlib.h"
	#include "BLO_readfile.h"

	#include "../../blender/windowmanager/WM_types.h"
	#include "../../blender/windowmanager/wm_window.h"
	#include "../../blender/windowmanager/wm_event_system.h"
}

void setupPythonGameloop(KX_KetsjiEngine *ketsjiengine, char *custom_loop);

KX_KetsjiEngine *setupKetsjiEngine(Scene *startscene, RAS_ICanvas *canvas, GlobalSettings *globalsettings, 
								   SCA_IInputDevice *keyboard, SCA_IInputDevice *mouse, NG_NetworkDeviceInterface *network,
								   KX_ISystem *system);
#endif