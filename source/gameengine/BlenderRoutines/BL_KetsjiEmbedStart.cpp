/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 * Blender's Ketsji startpoint
 */

/** \file gameengine/BlenderRoutines/BL_KetsjiEmbedStart.cpp
 *  \ingroup blroutines
 */


#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef _MSC_VER
   /* don't show stl-warnings */
#  pragma warning (disable:4786)
#endif

#include "GL/glew.h"

#include "KX_BlenderCanvas.h"
#include "KX_BlenderKeyboardDevice.h"
#include "KX_BlenderMouseDevice.h"
#include "KX_KetsjiSetupTools.h"
#include "KX_BlenderSystem.h"
#include "BL_Material.h"

#include "KX_KetsjiEngine.h"
#include "KX_BlenderSceneConverter.h"
#include "KX_PythonInit.h"
#include "KX_PyConstraintBinding.h"
#include "KX_PythonMain.h"
#include "KX_EngineCallbackData.h"
#include "KX_KetsjiLogicLoop.h"
#include "KX_PythonLogicLoop.h"

#include "RAS_GLExtensionManager.h"
#include "RAS_OpenGLRasterizer.h"
#include "RAS_ListRasterizer.h"

#include "NG_LoopBackNetworkDeviceInterface.h"

#include "BL_System.h"

#include "GPU_extensions.h"
#include "Value.h"


#ifdef WITH_AUDASPACE
#  include "AUD_C-API.h"
#  include "AUD_I3DDevice.h"
#  include "AUD_IDevice.h"
#endif

static BlendFileData *load_game_data(char *filename)
{
	ReportList reports;
	BlendFileData *blend_file_data;
	
	BKE_reports_init(&reports, RPT_STORE);
	blend_file_data = BLO_read_from_file(filename, &reports);

	if (!blend_file_data) {
		printf("Loading %s failed: ", filename);
		BKE_reports_print(&reports, RPT_ERROR);
	}

	BKE_reports_clear(&reports);

	return blend_file_data;
}

struct KX_EmbeddedCallbackData{
	// BL_Embedded player data container
	bool draw_letterbox;
	Scene *scene;
	ARegion *ar;
	wmWindow *win;
	KX_KetsjiEngine *ketsjiengine;
	bContext *C;

};

bool KX_RenderCallback(KX_EngineCallbackData *data)
{
	KX_EmbeddedCallbackData *embedded_data = (KX_EmbeddedCallbackData*)(data->data);
	if (embedded_data->draw_letterbox) {
			// Clear screen to border color
			// We do this here since we set the canvas to be within the frames. This means the engine
			// itself is unaware of the extra space, so we clear the whole region for it.
		GameData gm = embedded_data->scene->gm;
		ARegion *ar = embedded_data->ar;

		glClearColor(gm.framing.col[0], 
			gm.framing.col[1], 
			gm.framing.col[2], 1.0f);

		glViewport(ar->winrct.xmin, 
			ar->winrct.ymin,
			BLI_rcti_size_x(&ar->winrct), 
			BLI_rcti_size_y(&ar->winrct));

			glClear(GL_COLOR_BUFFER_BIT);
		}

	return true;
}
	
void KX_EventCallback(KX_EngineCallbackData *data)
{
	KX_EmbeddedCallbackData *embedded_data = (KX_EmbeddedCallbackData*)(data->data);

	wmWindow *win = embedded_data->win;
	ARegion *ar = embedded_data->ar;
	bContext *b_context = embedded_data->C;
	KX_KetsjiEngine *ketsjiengine = embedded_data->ketsjiengine;

	KX_BlenderKeyboardDevice *keyboarddevice = (KX_BlenderKeyboardDevice*)ketsjiengine->GetKeyboardDevice();
	KX_BlenderMouseDevice *mousedevice = (KX_BlenderMouseDevice*)ketsjiengine->GetMouseDevice();
	
	wm_window_process_events_nosleep();

	while (wmEvent *event= (wmEvent *)win->queue.first) {
		short val = 0;
		//unsigned short event = 0; //XXX extern_qread(&val);
		unsigned int unicode = event->utf8_buf[0] ? BLI_str_utf8_as_unicode(event->utf8_buf) : event->ascii;
		
		// test for the ESC key
		//XXX while (qtest())
		if (keyboarddevice->ConvertBlenderEvent(event->type, event->val, unicode))
			ketsjiengine->RequestExit(KX_EXIT_REQUEST_BLENDER_ESC);

		/* Coordinate conversion... where
		 * should this really be?
		 */
		if (event->type == MOUSEMOVE) {
			/* Note, not nice! XXX 2.5 event hack */
			val = event->x - ar->winrct.xmin;
			mousedevice->ConvertBlenderEvent(MOUSEX, val, 0);

			val = ar->winy - (event->y - ar->winrct.ymin) - 1;
			mousedevice->ConvertBlenderEvent(MOUSEY, val, 0);
		}
		else {
			mousedevice->ConvertBlenderEvent(event->type, event->val, 0);
		}

		BLI_remlink(&win->queue, event);
		wm_event_free(event);
	}

	if (win != CTX_wm_window(b_context)) {
		ketsjiengine->RequestExit(KX_EXIT_REQUEST_OUTSIDE); // window closed while bge runs
	}
}

void setupMaterials(GlobalSettings globalsettings, KX_ISceneConverter *sceneconverter, Scene *scene)
{
	bool usemat, useglslmat = false;
	if (GLEW_ARB_multitexture && GLEW_VERSION_1_1)
		usemat = true;
	if (GPU_glsl_support())
		useglslmat = true;
	else if (globalsettings.matmode == GAME_MAT_GLSL)
		usemat = false;

	if (usemat && (globalsettings.matmode != GAME_MAT_TEXFACE))
		sceneconverter->SetMaterials(true);
	if (useglslmat && (globalsettings.matmode == GAME_MAT_GLSL))
		sceneconverter->SetGLSLMaterials(true);
	if (scene->gm.flag & GAME_NO_MATERIAL_CACHING)
		sceneconverter->SetCacheMaterials(false);
}

BlendFileData *getBlendFileData(char *pathname, STR_String exitstring)
{
	char basedpath[FILE_MAX];
	// base the actuator filename with respect
	// to the original file working directory
	if (exitstring != "")
		BLI_strncpy(basedpath, exitstring.ReadPtr(), sizeof(basedpath));
	
	// load relative to the last loaded file, this used to be relative
	// to the first file but that makes no sense, relative paths in
	// blend files should be relative to that file, not some other file
	// that happened to be loaded first
	BLI_path_abs(basedpath, pathname);
	BlendFileData *blend_file_data = load_game_data(basedpath);

	// If the path didn't load, try a "relative" path
	if (!blend_file_data)
	{
		// just add "//" in front of it
		char temppath[242];
		strcpy(temppath, "//");
		strcat(temppath, basedpath);
		BLI_path_abs(temppath, pathname);
		blend_file_data = load_game_data(temppath);
	}
	if (!blend_file_data)
		return NULL;

	return blend_file_data;
}


extern "C" void StartKetsjiShell(struct bContext *C, struct ARegion *ar, rcti *cam_frame, int always_use_expand_framing)
{
	/* context values */
	struct wmWindowManager *wm= CTX_wm_manager(C);
	struct wmWindow *win= CTX_wm_window(C);
	struct Scene *startscene= CTX_data_scene(C);
	struct Main* maggie1= CTX_data_main(C);

	RAS_Rect area_rect;
	area_rect.SetLeft(cam_frame->xmin);
	area_rect.SetBottom(cam_frame->ymin);
	area_rect.SetRight(cam_frame->xmax);
	area_rect.SetTop(cam_frame->ymax);

	int exitrequested = KX_EXIT_REQUEST_NO_REQUEST;
	Main* blenderdata = maggie1;

	char* startscenename = startscene->id.name+2;
	char pathname[FILE_MAXDIR+FILE_MAXFILE], oldsce[FILE_MAXDIR+FILE_MAXFILE];
	STR_String exitstring = "";
	BlendFileData *blend_file_data= NULL;

	BLI_strncpy(pathname, blenderdata->name, sizeof(pathname));
	BLI_strncpy(oldsce, G.main->name, sizeof(oldsce));

#ifdef WITH_PYTHON
	resetGamePythonPath(); // need this so running a second time wont use an old blendfiles path
	setGamePythonPath(G.main->name);

	// Acquire Python's GIL (global interpreter lock)
	// so we can safely run Python code and API calls
	PyGILState_STATE gilstate = PyGILState_Ensure();
	
	PyObject *pyGlobalDict = PyDict_New(); /* python utility storage, spans blend file loading */
#endif
	
	bgl::InitExtensions(true);

	// VBO code for derived mesh is not compatible with BGE (couldn't find why), so disable
	int disableVBO = (U.gameflags & USER_DISABLE_VBO);
	U.gameflags |= USER_DISABLE_VBO;

	// Globals to be carried on over blender files
	GlobalSettings gs;
	gs.matmode= startscene->gm.matmode;
	gs.glslflag= startscene->gm.flag;
				
	// Create callbacks for render setup and event setup pre engine
	KX_EngineCallbackData *callbacks = new KX_EngineCallbackData();
	KX_EmbeddedCallbackData *callback_data = new KX_EmbeddedCallbackData();

	callbacks->rendercallback = KX_RenderCallback;
	callbacks->eventcallback = KX_EventCallback;

	// User defined Python gameloop path
	char *custom_loop = startscene->gm.custom_loop;
	cout << "callbacks made" << endl;
	do
	{
		View3D *v3d= CTX_wm_view3d(C);
		RegionView3D *rv3d= CTX_wm_region_view3d(C);

		// Create the canvas
		RAS_ICanvas* canvas = new KX_BlenderCanvas(wm, win, area_rect, ar);

		// Setup vsync
		int previous_vsync = canvas->GetSwapInterval();
				
		// Create the inputdevices
		KX_BlenderKeyboardDevice* keyboard = new KX_BlenderKeyboardDevice();
		KX_BlenderMouseDevice* mouse = new KX_BlenderMouseDevice();
		
		// Create a networkdevice
		NG_NetworkDeviceInterface* network = new NG_LoopBackNetworkDeviceInterface();

		// Create a ketsji/blendersystem (only needed for timing and stuff)
		KX_BlenderSystem* kxsystem = new KX_BlenderSystem();
		KX_KetsjiEngine *ketsjiengine = setupKetsjiEngine(startscene, canvas, &gs, keyboard, mouse, network, kxsystem);
			
		// Get the rasterizer (from engine creation call)
		RAS_IRasterizer *rasterizer = ketsjiengine->GetRasterizer();
		RAS_IRasterizer::MipmapOption mipmapval = rasterizer->GetMipmapping();

		//lock frame and camera enabled - storing global values
		int tmp_lay= startscene->lay;
		Object *tmp_camera = startscene->camera;

		if (v3d->scenelock==0) {
			startscene->lay= v3d->lay;
			startscene->camera= v3d->camera;
		}

		// some blender stuff
		float camzoom = 2.0;
		int draw_letterbox = 0;
		
		if (rv3d->persp==RV3D_CAMOB) {
			if (startscene->gm.framing.type == SCE_GAMEFRAMING_BARS) { /* Letterbox */
				camzoom = 1.0f;
				draw_letterbox = 1;
			}
			else
				camzoom = 1.0f / BKE_screen_view3d_zoom_to_fac(rv3d->camzoom);
			}

		// Set the drawing type from the 3D viewport
		ketsjiengine->SetDrawType(v3d->drawtype);
		ketsjiengine->SetCameraZoom(camzoom);
		
		// if we got an exitcode 3 (KX_EXIT_REQUEST_START_OTHER_GAME) load a different file
		if (exitrequested == KX_EXIT_REQUEST_START_OTHER_GAME || exitrequested == KX_EXIT_REQUEST_RESTART_GAME)
		{
			exitrequested = KX_EXIT_REQUEST_NO_REQUEST;

			if (blend_file_data) 
				BLO_blendfiledata_free(blend_file_data);
			
			// Get the blend file data from path
			blend_file_data = getBlendFileData(pathname, exitstring);
				
			// if we got a loaded blendfile, proceed
			if (blend_file_data)
			{
				blenderdata = blend_file_data->main;
				startscenename = blend_file_data->curscene->id.name + 2;

				if (blenderdata) {
					BLI_strncpy(G.main->name, blenderdata->name, sizeof(G.main->name));
					BLI_strncpy(pathname, blenderdata->name, sizeof(pathname));
#ifdef WITH_PYTHON
					setGamePythonPath(G.main->name);
#endif
				}
			}
			// Else we cannot do anything, so exit!
			else
				exitrequested = KX_EXIT_REQUEST_QUIT_GAME;
			}

		// Get a scene to start with
		Scene *scene = blend_file_data ? blend_file_data->curscene : (Scene *)BLI_findstring(&blenderdata->scene, startscenename, offsetof(ID, name) + 2);

		// If it exists, setup scene
		if (scene)
		{
			// Start animation recording from scene's current frame (if enabled)
			ketsjiengine->SetAnimationFrame(scene->r.cfra);

			// Setup stero rendering if enabled
			if (scene->gm.stereoflag == STEREO_ENABLED) {
			// Quad buffered needs a special window.
				if (scene->gm.stereomode != RAS_IRasterizer::RAS_STEREO_QUADBUFFERED)
					rasterizer->SetStereoMode((RAS_IRasterizer::StereoMode) scene->gm.stereomode);
				// Setup stereo eye separation
				rasterizer->SetEyeSeparation(scene->gm.eyeseparation);
			}

			// Set background color of window from scene
			rasterizer->SetBackColor(scene->gm.framing.col[0], scene->gm.framing.col[1], scene->gm.framing.col[2], 0.0f);
		}
		
		if (exitrequested != KX_EXIT_REQUEST_QUIT_GAME)
		{
			if (rv3d->persp != RV3D_CAMOB)
			{
				ketsjiengine->EnableCameraOverride(startscenename);
				ketsjiengine->SetCameraOverrideUseOrtho((rv3d->persp == RV3D_ORTHO));
				ketsjiengine->SetCameraOverrideProjectionMatrix(MT_CmMatrix4x4(rv3d->winmat));
				ketsjiengine->SetCameraOverrideViewMatrix(MT_CmMatrix4x4(rv3d->viewmat));
					ketsjiengine->SetCameraOverrideClipping(v3d->near, v3d->far);
				ketsjiengine->SetCameraOverrideLens(v3d->lens);
			}
			
			// Create the scene converter
			KX_ISceneConverter* sceneconverter = new KX_BlenderSceneConverter(blenderdata, ketsjiengine);

			// Set the scene converter
			ketsjiengine->SetSceneConverter(sceneconverter);
			sceneconverter->addInitFromFrame = false;

			if (always_use_expand_framing)
				sceneconverter->SetAlwaysUseExpandFraming(true);

			// Setup the materials for the sceneconverter
			setupMaterials(gs, sceneconverter, scene);
					
			// Create the initial scene
			KX_Scene* startscene = new KX_Scene(ketsjiengine->GetKeyboardDevice(), ketsjiengine->GetMouseDevice(),
												ketsjiengine->GetNetworkDevice(), startscenename, scene, canvas);

#ifdef WITH_PYTHON
			// Create namespaces
			PyObject *gameLogic, *gameLogic_keys;
			// Setup Python API imports
			setupGamePython(ketsjiengine, startscene, blenderdata, pyGlobalDict, &gameLogic, &gameLogic_keys, 0, NULL);
			// Setup GameLoop from Python
			setupPythonGameloop(ketsjiengine, custom_loop);
			
#endif // WITH_PYTHON
			cout << "MAINSETUP" << endl;

			//initialize Dome Settings
			if (scene->gm.stereoflag == STEREO_DOME)
				ketsjiengine->InitDome(scene->gm.dome.res, scene->gm.dome.mode, scene->gm.dome.angle, scene->gm.dome.resbuf, scene->gm.dome.tilt, scene->gm.dome.warptext);

			// initialize 3D Audio Settings
			AUD_I3DDevice* dev = AUD_get3DDevice();
			if (dev)
			{
				dev->setSpeedOfSound(scene->audio.speed_of_sound);
				dev->setDopplerFactor(scene->audio.doppler_factor);
				dev->setDistanceModel(AUD_DistanceModel(scene->audio.distance_model));
			}

			// from see blender.c:
			// FIXME: this version patching should really be part of the file-reading code,
			// but we still get too many unrelated data-corruption crashes otherwise...
			if (blenderdata->versionfile < 250)
				do_versions_ipos_to_animato(blenderdata);

			// Convert starting scene
			if (sceneconverter)
			{
				// convert and add scene
				sceneconverter->ConvertScene(startscene, rasterizer, canvas);
				ketsjiengine->AddScene(startscene);
				
				// init the rasterizer
				rasterizer->Init();
				
				// start the engine
				ketsjiengine->StartEngine(true);

				// Set the animation playback rate for ipo's and actions
				// the framerate below should patch with FPS macro defined in blendef.h
				// Could be in StartEngine set the framerate, we need the scene to do this
				ketsjiengine->SetAnimFrameRate(FPS);

				callback_data->C = C;
				callback_data->win = win;
				callback_data->scene = scene;
				callback_data->ar = ar;
				callback_data->draw_letterbox = draw_letterbox;
				callback_data->ketsjiengine = ketsjiengine;

				// Pass callbacks to refresh display, events
				ketsjiengine->SetEngineCallbacks(callbacks);

				// Starting gameloop
				printf("\nBlender Game Engine Started\n");

				// Allow engine to run loop
				KX_KetsjiLogicLoop *loop = ketsjiengine->GetLogicLoop();
				loop->GiveHandle();
				loop = NULL;

				// Gameloop now exiting
				printf("Blender Game Engine Finished\n");

				// Get exit data
				exitstring = ketsjiengine->GetExitString();
				exitrequested = ketsjiengine->GetExitCode();

				gs = *(ketsjiengine->GetGlobalSettings());

#ifdef WITH_PYTHON
				/* Clears the dictionary by hand:
				This prevents, extra references to global variables	inside the
				GameLogic dictionary when the python interpreter is finalized.
				This allows the scene to safely delete them; see (space.c)->start_game
				PyDict_Clear(PyModule_GetDict(gameLogic)); */
				
				// Keep original items, means python plugins will autocomplete members
				PyObject *new_gameLogic_keys = PyDict_Keys(PyModule_GetDict(gameLogic));
				const Py_ssize_t numitems= PyList_GET_SIZE(new_gameLogic_keys);
				
				for (Py_ssize_t index=0; index < numitems; index++) {
					PyObject *item = PyList_GET_ITEM(new_gameLogic_keys, index);

					if (!PySequence_Contains(gameLogic_keys, item)) {
						PyDict_DelItem(PyModule_GetDict(gameLogic), item);
					}
				}

				Py_DECREF(new_gameLogic_keys);
				new_gameLogic_keys = NULL;
#endif
				ketsjiengine->StopEngine();
#ifdef WITH_PYTHON
				exitGamePythonScripting();
#endif
				delete sceneconverter;
				sceneconverter = NULL;
			}

#ifdef WITH_PYTHON
			Py_DECREF(gameLogic_keys);
			gameLogic_keys = NULL;
#endif
		}
		//lock frame and camera enabled - restoring global values
		if (v3d->scenelock==0) {
			startscene->lay= tmp_lay;
			startscene->camera= tmp_camera;
		}

		// Reset mouse state
		if (exitrequested != KX_EXIT_REQUEST_OUTSIDE)
		{
			// set the cursor back to normal
			canvas->SetMouseState(RAS_ICanvas::MOUSE_NORMAL);

			// set mipmap setting back to its original value
			rasterizer->SetMipmapping(mipmapval);
		}
		
		// clean up some stuff
		if (ketsjiengine)
		{
			delete ketsjiengine;
			ketsjiengine = NULL;
		}
		if (callbacks)
		{
			delete callbacks;
			callbacks = NULL;
		}
		if (callback_data)
		{
			delete callback_data;
			callback_data = NULL;
		}
		if (rasterizer)
		{
			rasterizer = NULL;
		}
		if (canvas)
		{
			canvas->SetSwapInterval(previous_vsync); // Set the swap interval back
			delete canvas;
			canvas = NULL;
		}

		// stop all remaining playing sounds
		AUD_getDevice()->stopAll();
	
	} while (exitrequested == KX_EXIT_REQUEST_RESTART_GAME || exitrequested == KX_EXIT_REQUEST_START_OTHER_GAME);
	
	if (!disableVBO)
		U.gameflags &= ~USER_DISABLE_VBO;

	if (blend_file_data) 
		BLO_blendfiledata_free(blend_file_data);

	BLI_strncpy(G.main->name, oldsce, sizeof(G.main->name));

#ifdef WITH_PYTHON
	// Decrement the globalDict reference
	Py_DECREF(pyGlobalDict);

	// Release Python's GIL
	PyGILState_Release(gilstate);
#endif

}
