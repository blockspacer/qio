/*
============================================================================
Copyright (C) 2013 V.

This file is part of Qio source code.

Qio source code is free software; you can redistribute it 
and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

Qio source code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA,
or simply visit <http://www.gnu.org/licenses/>.
============================================================================
*/
// btp_staticMapLoader.cpp
#include "btp_convert.h"
#include "btp_staticMapLoader.h"
#include "btp_world.h"
#include "btp_cMod2BulletShape.h"
#include <api/vfsAPI.h>
#include <api/cmAPI.h>
#include <shared/bspPhysicsDataLoader.h>
#include <shared/str.h>

static class bspPhysicsDataLoader_c *g_bspPhysicsLoader = 0;
static class btpStaticMapLoader_c *g_staticMap = 0;

// brush converting
static btAlignedObjectArray<btVector3> planeEquations;
static void BT_AddBrushPlane(const float q3Plane[4]) {
	btVector3 planeEq;
	planeEq.setValue(q3Plane[0],q3Plane[1],q3Plane[2]);
	// q3 plane equation is Ax + By + Cz - D = 0, so negate D
	planeEq[3] = -q3Plane[3];
	planeEquations.push_back(planeEq);
}
void BT_ConvertWorldBrush(u32 brushNum, u32 contentFlags) {
	if((contentFlags & 1) == 0)
		return;
	planeEquations.clear();
	g_bspPhysicsLoader->iterateBrushPlanes(brushNum,BT_AddBrushPlane);
	// convert plane equations -> vertex cloud
	btAlignedObjectArray<btVector3>	vertices;
	btGeometryUtil::getVerticesFromPlaneEquations(planeEquations,vertices);
	BT_ConvertVerticesArrayFromQioToBullet(vertices);
	g_staticMap->createWorldBrush(vertices);
}
btpStaticMapLoader_c::btpStaticMapLoader_c() {
	mainWorldShape = 0;
	mainWorldBody = 0;
}
bool btpStaticMapLoader_c::loadFromBSPFile(const char *fname) {
	bspPhysicsDataLoader_c l;		
	if(l.loadBSPFile(fname)) {
		return true;
	}
	g_bspPhysicsLoader = &l;
	g_staticMap = this;
	// load static world geometry
	if(l.isCoD1BSP() || l.isHLBSP()) {
		// HL bsps dont have brush data
		// COD bsps have brush data, but we havent reverse engineered it fully yet
		//l.iterateModelTriSurfs(0,BT_ConvertWorldPoly);
	} else {
		l.iterateModelBrushes(0,BT_ConvertWorldBrush);
		//l.iterateModelBezierPatches(0,BT_ConvertWorldBezierPatch);
	}
	return false;
}
bool btpStaticMapLoader_c::loadFromMAPFile(const char *fname) {
	cMod_i *m = cm->registerModel(fname);
	if(m == 0)
		return true; // error
	mainWorldShape = BT_CModelToBulletCollisionShape(m,true);
	if(mainWorldShape == 0) {
		return true; // error
	}
	mainWorldBody = new btRigidBody(0,0,mainWorldShape,btVector3(0,0,0));	
	myPhysWorld->getBTDynamicsWorld()->addRigidBody(mainWorldBody);
	return false;
}
void btpStaticMapLoader_c::createWorldBrush(const btAlignedObjectArray<btVector3> &vertices) {
	// create convex hull shape
	class btConvexHullShape *shape = BT_ConvexHullShapeFromVerticesArray(vertices);
	this->shapes.push_back(shape);
	// create static body
	btRigidBody* body = new btRigidBody(0,0,shape,btVector3(0,0,0));	
	myPhysWorld->getBTDynamicsWorld()->addRigidBody(body);
	this->bodies.push_back(body);
}
bool btpStaticMapLoader_c::loadMap(const char *mapName, class bulletPhysicsWorld_c *pWorld) {
	this->myPhysWorld = pWorld;
	str path = "maps/";
	path.append(mapName);
	path.setExtension("bsp");
	if(g_vfs->FS_FileExists(path)) {
		return loadFromBSPFile(path);
	}
	path.setExtension("proc");
	if(g_vfs->FS_FileExists(path)) {
		//return loadPROCFile(path);
	}
	path.setExtension("map");
	if(g_vfs->FS_FileExists(path)) {
		return loadFromMAPFile(path);
	}
	return false; // no error
}
void btpStaticMapLoader_c::freeMemory() {
	for(u32 i = 0; i < shapes.size(); i++) {
		delete shapes[i];
	}
	shapes.clear();
	for(u32 i = 0; i < bodies.size(); i++) {
		myPhysWorld->getBTDynamicsWorld()->removeRigidBody(bodies[i]);
		delete bodies[i];
	}
	bodies.clear();
	if(mainWorldShape) {
		delete mainWorldShape;
		mainWorldShape = 0;
	}
	if(mainWorldBody) {
		myPhysWorld->getBTDynamicsWorld()->removeRigidBody(mainWorldBody);
		delete mainWorldBody;
		mainWorldBody = 0;
	}
}