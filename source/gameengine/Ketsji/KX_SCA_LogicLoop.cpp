#include "KX_SCA_LogicLoop.h"
#include "KX_KetsjiEngine.h"
#include "KX_KetsjiLogicLoop.h"
#include "PHY_IPhysicsEnvironment.h"
#include "KX_PyConstraintBinding.h"
#include "KX_TimeCategoryLogger.h"
#include "KX_PythonInit.h"
#include "NG_NetworkScene.h"
#include "NG_NetworkDeviceInterface.h"
#include "KX_ISceneConverter.h"
#include "SCA_IInputDevice.h"

KX_SCA_LogicLoop::KX_SCA_LogicLoop(KX_KetsjiEngine* engine) : KX_KetsjiLogicLoop(engine)
{
};


void KX_SCA_LogicLoop::GiveHandle()
{
	while (!m_engine->GetExitCode())
	{
		// Update keyboard and mouse events
		m_engine->UpdateEvents();
		NextFrame();
	}
};

void KX_SCA_LogicLoop::NextFrame()
{
	KX_TimeCategoryLogger* logger = m_engine->GetLogger();

	double timestep = 1.0 / m_engine->GetTicRate();
	double framestep = timestep;
	
	// Start time logging for "services"
	StartProfile(tc_services);

	// Increment engine time by fixed timestep
	if (m_engine->GetUseFixedTime()) {
		m_engine->SetClockTime(m_engine->GetClockTime() + timestep);
	}
	// Or use real deltatime
	else 
	{
		m_engine->SetClockTime(GetTime());
	}

	// Get deltatime duration
	double deltatime = m_engine->GetClockTime() - m_engine->GetFrameTime();
	
	// We got here too quickly, which means there is nothing todo, just return and don't render.
	// Not sure if this is the best fix, but it seems to stop the jumping framerate issue (#33088)
	if (deltatime < 0.f)
		return;

	// Compute the number of logic frames to do each update (fixed tic bricks)
	int frames = int(deltatime * m_engine->GetTicRate() + 1e-6);
	
	int maxPhysicsFrames = m_engine->GetMaxPhysicsFrame();
	// First, enforce a limit on the number of frames according to physics
	if (frames > maxPhysicsFrames)
	{
		m_engine->SetFrameTime(m_engine->GetFrameTime() + (frames - maxPhysicsFrames) * timestep);
		frames = maxPhysicsFrames;
	}

	// Assert we want to render
	bool doRender = frames > 0;

	int maxLogicFrames = m_engine->GetMaxLogicFrame();

	// Now enforce limit on frames according to logic maximum
	if (frames > maxLogicFrames)
	{
		framestep = (frames * timestep) / maxLogicFrames;
		frames = maxLogicFrames;
	}

	// While we have frames to update with
	double frametime;

	while (frames)
	{
		frametime = m_engine->GetFrameTime() + framestep;
		m_engine->SetFrameTime(frametime);

		KX_SceneList* scenes = m_engine->CurrentScenes();

		int scene_count = scenes->size();

		for (int i=0; i < scene_count; i++)
		// for each scene, call the proceed functions
		{
			KX_Scene* scene = scenes->at(i);
	
			/* Suspension holds the physics and logic processing for an
			 * entire scene. Objects can be suspended individually, and
			 * the settings for that precede the logic and physics
			 * update. */
			StartProfile(tc_logic);

			m_engine->GetSceneConverter()->resetNoneDynamicObjectToIpo();//this is for none dynamic objects with ipo

			scene->UpdateObjectActivity();
	
			if (!scene->IsSuspended())
			{
				// if the scene was suspended recalcutlate the delta tu "curtime"
				m_engine->SetSuspendedTime(scene->getSuspendedTime());

				if (scene->getSuspendedTime()!=0.0)
					scene->setSuspendedDelta(scene->getSuspendedDelta() + m_engine->GetClockTime()-scene->getSuspendedTime());

				m_engine->SetSuspendedDelta(scene->getSuspendedDelta());

				StartProfile(tc_network);
				SG_SetActiveStage(SG_STAGE_NETWORK);
				ReceiveMessages(frametime);
				
				StartProfile(tc_physics);
				SG_SetActiveStage(SG_STAGE_PHYSICS1);

				// set Python hooks for each scene
				SetCurrentScene(scene);
	
				// Update scenegraph after physics step. This maps physics calculations
				// into node positions.
				scene->GetPhysicsEnvironment()->EndFrame();
				
				UpdateLogicBricks(frametime);

				if (!m_engine->GetRestrictAnimationFPS())
				{
					StartProfile(tc_animations);
					SG_SetActiveStage(SG_STAGE_ANIMATION_UPDATE);
					UpdateAnimations(frametime);
				}

				StartProfile(tc_physics);
				SG_SetActiveStage(SG_STAGE_PHYSICS2);
				scene->GetPhysicsEnvironment()->BeginFrame();
		
				// Perform physics calculations on the scene. This can involve 
				// many iterations of the physics solver.
				scene->GetPhysicsEnvironment()->ProceedDeltaTime(frametime, timestep, framestep);//m_deltatime realDeltaTime);

				StartProfile(tc_scenegraph);
				SG_SetActiveStage(SG_STAGE_PHYSICS2_UPDATE);
				scene->UpdateParents(frametime);
			
				if (m_engine->GetAnimationRecord())
				{
					int next_frame = m_engine->GetAnimationFrame() + 1;
					// Set as next frame
					m_engine->SetAnimationFrame(next_frame);
					// Record this frame
					m_engine->GetSceneConverter()->WritePhysicsObjectToAnimationIpo(next_frame);
				}

				scene->setSuspendedTime(0.0);
			} // suspended
			else
				if (scene->getSuspendedTime()==0.0)
					scene->setSuspendedTime(m_engine->GetClockTime());
			
			StartProfile(tc_services);
		}

		StartProfile(tc_outside);
		
		// update system devices
		UpdateKeyboard();
		UpdateMouse();
		UpdateMessages();

		// scene management
		UpdateScenes();
		
		frames--;
	}

	if (m_engine->GetRestrictAnimationFPS())
	{
		double clocktime = GetTime();

		StartProfile(tc_animations, clocktime);
		SG_SetActiveStage(SG_STAGE_ANIMATION_UPDATE);

		double anim_timestep = 1.0 / KX_GetActiveScene()->GetAnimationFPS();

		if (clocktime - m_engine->GetPreviousAnimTime() > anim_timestep)
		{
			m_engine->SetPreviousAnimTime(clocktime);

			KX_SceneList* scenes = m_engine->CurrentScenes();
			int scene_count = scenes->size();
			for (int i=0; i < scene_count; i++)
			// for each scene, call the proceed functions
			{
				KX_Scene* scene = scenes->at(i);
				// update the animations for the "current scene"
				SetCurrentScene(scene);
				UpdateAnimations(clocktime);
			}
		}
	}
	
	StartProfile(tc_outside);

	// If we want to render
	if (doRender)
	{
		StartProfile(tc_rasterizer);
		UpdateRender();
	}
};