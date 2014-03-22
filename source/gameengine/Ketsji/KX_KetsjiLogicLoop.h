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

/** \file KX_KetsjiLogicLoop.h
 *  \ingroup ketsji
 */

#ifndef __KX_KETSJILOGICLOOP_H__
#define __KX_KETSJILOGICLOOP_H__

#include "KX_Python.h"
#include "KX_WorldInfo.h"
#include "KX_KetsjiEngine.h"

class KX_KetsjiLogicLoop
{
public:
	KX_KetsjiLogicLoop(KX_KetsjiEngine *engine);
	//virtual ~KX_KetsjiLogicLoop()=0;
	KX_KetsjiEngine *m_engine;

	virtual void GiveHandle() = 0;
	void UpdateKeyboard();
	void UpdateMouse();
	void UpdateMessages();
	void UpdateBlender();
	void UpdateRender();
	void UpdateScenes();
	void StartProfile(int profile_id, double currenttime=NULL, bool stop_others=true);
	void StopProfile(int profile_id, double currenttime=NULL);
	void SetCurrentScene(KX_Scene *scene);
	void ReceiveMessages(double currenttime);
	void UpdateLogicBricks(double current_time);
	void UpdateScenegraph(double currenttime);
	void UpdateAnimations(double currenttime);
	void UpdatePhysics(double current_time, double fixed_step_time, double delta_time);
	KX_Scene *GetCurrentScene();
	double GetTime();
};
#endif  /* __KX_KETSJILOGICLOOP_H__ */