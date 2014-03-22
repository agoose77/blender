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
 *
 */

/** \file gameengine/Ketsji/KX_KetsjiLogicLoop.cpp
 *  \ingroup ketsji
 */

#ifdef _MSC_VER
#  pragma warning (disable:4786)
#endif

#include <iostream>
#include <stdio.h>

#include "KX_KetsjiLogicLoop.h"
#include "PHY_IPhysicsEnvironment.h"
#include "KX_PyConstraintBinding.h"
#include "KX_KetsjiEngine.h"
#include "KX_TimeCategoryLogger.h"
#include "KX_Scene.h"
#include "SCA_IInputDevice.h"
#include "NG_NetworkScene.h"
#include "KX_ISceneConverter.h"
#include "KX_PythonInit.h"
#include "NG_NetworkDeviceInterface.h"

KX_KetsjiLogicLoop::KX_KetsjiLogicLoop(KX_KetsjiEngine* engine):
m_engine(engine)
{};

// Start profiling this category
void KX_KetsjiLogicLoop::StartProfile(int profile_id, double currenttime, bool stop_others)
{
	if (currenttime == NULL)
		currenttime = GetTime();

	m_engine->GetLogger()->StartLog(profile_id, currenttime, stop_others);
}

// Stop profiling this category
void KX_KetsjiLogicLoop::StopProfile(int profile_id, double currenttime)
{
	if (currenttime == NULL)
		currenttime = GetTime();

	m_engine->GetLogger()->EndLog(profile_id, currenttime);
}

// Returns the current engine time
double KX_KetsjiLogicLoop::GetTime() 
{
	return m_engine->GetRealTime(); 
};

// Asks engine to render screen
void KX_KetsjiLogicLoop::UpdateRender()
{
	m_engine->Render();
}

// Update the blender events
void KX_KetsjiLogicLoop::UpdateBlender()
{
	m_engine->UpdateEvents();
}

// Update the logic brick sytems
void KX_KetsjiLogicLoop::UpdateLogicBricks(double current_time)
{
	KX_Scene *scene = KX_GetActiveScene();

	// Process sensors, and controllers
	StartProfile(tc_logic);
	SG_SetActiveStage(SG_STAGE_CONTROLLER);
	scene->LogicBeginFrame(current_time);
	
	// Scenegraph needs to be updated again, because Logic Controllers 
	// can affect the local matrices.
	StartProfile(tc_scenegraph);
	SG_SetActiveStage(SG_STAGE_CONTROLLER_UPDATE);

	UpdateScenegraph(current_time);
	
	// Process actuators
	// Do some cleanup work for this logic frame
	StartProfile(tc_logic);
	SG_SetActiveStage(SG_STAGE_ACTUATOR);

	scene->LogicUpdateFrame(current_time, true);
	scene->LogicEndFrame();
	
	// Actuators can affect the scenegraph
	StartProfile(tc_scenegraph);
	SG_SetActiveStage(SG_STAGE_ACTUATOR_UPDATE);
	UpdateScenegraph(current_time);
}

// Asks engine to update the message device
void KX_KetsjiLogicLoop::UpdateMessages()
{
if (m_engine->GetNetworkDevice())
	m_engine->GetNetworkDevice()->NextFrame();
}

// Asks engine to update mouse events from blender
void KX_KetsjiLogicLoop::UpdateMouse()
{
	if (m_engine->GetMouseDevice())
		m_engine->GetMouseDevice()->NextFrame();
}

// Ask engine to update keyboard events from blender
void KX_KetsjiLogicLoop::UpdateKeyboard()
{

	if (m_engine->GetKeyboardDevice())
		m_engine->GetKeyboardDevice()->NextFrame();
}

// Sets the scene context for context-dependant operations
void KX_KetsjiLogicLoop::SetCurrentScene(KX_Scene *scene)
{
#ifdef WITH_PYTHON
	PHY_SetActiveEnvironment(scene->GetPhysicsEnvironment());
#endif
	KX_SetActiveScene(scene);
}

KX_Scene *KX_KetsjiLogicLoop::GetCurrentScene()
{
	return KX_GetActiveScene();
}

// Updates scenegraph in context of current scene
void KX_KetsjiLogicLoop::UpdateScenegraph(double currenttime)
{
	KX_GetActiveScene()->UpdateParents(currenttime);
}

// Updates scenes that were added/destroyed/replaced
void KX_KetsjiLogicLoop::UpdateScenes()
{
	m_engine->ProcessScheduledScenes();
}

// Receives messages from the "network"
void KX_KetsjiLogicLoop::ReceiveMessages(double currenttime)
{
	KX_GetActiveScene()->GetNetworkScene()->proceed(currenttime);
}

// Updates animations in context of current scene
void KX_KetsjiLogicLoop::UpdateAnimations(double currenttime)
{
	KX_GetActiveScene()->UpdateAnimations(currenttime);
}

// Asks engine to update physics in context of current scene
void KX_KetsjiLogicLoop::UpdatePhysics(double current_time, double fixed_step_time, double delta_time)
{
	if (delta_time == NULL)
		delta_time = fixed_step_time;

	KX_GetActiveScene()->GetPhysicsEnvironment()->ProceedDeltaTime(current_time, fixed_step_time, delta_time);

};