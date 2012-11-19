/*
============================================================================
Copyright (C) 2012 V.

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
// rf_surface.cpp - static surface class
#include "rf_surface.h"
#include "rf_drawCall.h"
#include <api/rbAPI.h>
#include <api/materialSystemAPI.h>
#include <api/mtrAPI.h>
#include <api/skelModelAPI.h>
#include <api/coreAPI.h>
#include <shared/trace.h>
#include <shared/parser.h> // for Doom3 .proc surfaces parsing
#include "rf_decalProjector.h"

//
//	r_surface_c class
//
r_surface_c::r_surface_c() {
	if(g_ms) {
		mat = g_ms->registerMaterial("defaultMaterial");
	} else {
		mat = 0;
	}
	lightmap = 0;
}
r_surface_c::~r_surface_c() {
	this->clear();
}
void r_surface_c::addTriangle(const struct simpleVert_s &v0, const struct simpleVert_s &v1, const struct simpleVert_s &v2) {
	indices.addIndex(verts.size());
	indices.addIndex(verts.size()+1);
	indices.addIndex(verts.size()+2);
	rVert_c nv;
	nv.xyz = v0.xyz;
	nv.tc = v0.tc;
	verts.push_back(nv);
	nv.xyz = v1.xyz;
	nv.tc = v1.tc;
	verts.push_back(nv);
	nv.xyz = v2.xyz;
	nv.tc = v2.tc;
	verts.push_back(nv);
	bounds.addPoint(v0.xyz);
	bounds.addPoint(v1.xyz);
	bounds.addPoint(v2.xyz);
}	
#include <shared/simpleTexturedPoly.h>
void r_surface_c::addPoly(const simplePoly_s &poly) {
	for(u32 i = 2; i < poly.verts.size(); i++) {
		addTriangle(poly.verts[0],poly.verts[i-1],poly.verts[i]);
	}
}
void r_surface_c::resizeVerts(u32 newNumVerts) {
	verts.resize(newNumVerts);
}
void r_surface_c::setVert(u32 vertexIndex, const struct simpleVert_s &v) {
	rVert_c &rv = verts[vertexIndex];
	rv.xyz = v.xyz;
	rv.tc = v.tc;
}
void r_surface_c::resizeIndices(u32 newNumIndices) {
	// TODO: see if we can use u16 buffer here
	indices.initU32(newNumIndices);
}
void r_surface_c::setIndex(u32 indexNum, u32 value) {
	indices.setIndex(indexNum,value);
}
void r_surface_c::setMaterial(mtrAPI_i *newMat) {
#if 1
	if(newMat == 0) {
		newMat = g_ms->registerMaterial("noMaterial");
	}
#endif
	mat = newMat;
	if(newMat == 0) {
		matName = "noMaterial";
	} else {
		matName = newMat->getName();
	}
}
void r_surface_c::setMaterial(const char *newMatName) {
	matName = newMatName;
	mat = g_ms->registerMaterial(newMatName);
}
void r_surface_c::drawSurface() {
	rb->setBindVertexColors(true);
	rb->setMaterial(this->mat,this->lightmap);
	rb->drawElements(this->verts,this->indices);
	rb->setBindVertexColors(false);
}
void r_surface_c::drawSurfaceWithSingleTexture(class textureAPI_i *tex) {
	rb->setBindVertexColors(false);
	rb->drawElementsWithSingleTexture(this->verts,this->indices,tex);
}

void r_surface_c::addDrawCall() {
	RF_AddDrawCall(&this->verts,&this->indices,this->mat,this->lightmap,this->mat->getSort(),true);
}
#include <api/colMeshBuilderAPI.h>
void r_surface_c::addGeometryToColMeshBuilder(class colMeshBuilderAPI_i *out) {
#if 0
	for(u32 i = 0; i < indices.getNumIndices(); i+=3) {
		u32 i0 = indices[i+0];
		u32 i1 = indices[i+1];
		u32 i2 = indices[i+2];
		const rVert_c &v0 = verts[i0];
		const rVert_c &v1 = verts[i1];
		const rVert_c &v2 = verts[i2];
		out->addXYZTri(v0.xyz,v1.xyz,v2.xyz);
	}
#else
	out->addMesh(verts.getArray()->xyz,sizeof(rVert_c),verts.size(),
		indices.getArray(),indices.is32Bit(),indices.getNumIndices());
#endif
}
bool r_surface_c::traceRay(class trace_c &tr) {
	if(tr.getTraceBounds().intersect(this->bounds) == false)
		return false;
	bool hasHit = false;
	for(u32 i = 0; i < indices.getNumIndices(); i+=3) {
		u32 i0 = indices[i+0];
		u32 i1 = indices[i+1];
		u32 i2 = indices[i+2];
		const rVert_c &v0 = verts[i0];
		const rVert_c &v1 = verts[i1];
		const rVert_c &v2 = verts[i2];
#if 1
		aabb tmpBB;
		tmpBB.fromTwoPoints(v0.xyz,v1.xyz);
		tmpBB.addPoint(v2.xyz);
		if(tmpBB.intersect(tr.getTraceBounds()) == false) {
			continue;
		}
#endif
		if(tr.clipByTriangle(v0.xyz,v1.xyz,v2.xyz,true)) {
			hasHit = true;
		}
	}
	return hasHit;
}	
bool r_surface_c::createDecalInternal(class decalProjector_c &proj) {
	u32 newPoints = 0;
	for(u32 i = 0; i < indices.getNumIndices(); i+=3) {
		u32 i0 = indices[i+0];
		u32 i1 = indices[i+1];
		u32 i2 = indices[i+2];
		const rVert_c &v0 = verts[i0];
		const rVert_c &v1 = verts[i1];
		const rVert_c &v2 = verts[i2];
		newPoints += proj.clipTriangle(v0.xyz,v1.xyz,v2.xyz);
	}
	return newPoints;
}
void r_surface_c::initSkelSurfInstance(const skelSurfaceAPI_i *skelSF) {
	clear();
	setMaterial(skelSF->getMatName());
	verts.resize(skelSF->getNumVerts());
	rVert_c *v = verts.getArray();
	const skelVert_s *inV = skelSF->getVerts();
	for(u32 i = 0; i < verts.size(); i++, v++, inV++) {
		v->tc = inV->tc;
	}
	indices.addU16Array(skelSF->getIndices(),skelSF->getNumIndices());
}
void r_surface_c::updateSkelSurfInstance(const class skelSurfaceAPI_i *skelSF, const class boneOrArray_c &bones) {
	rVert_c *v = verts.getArray();
	const skelVert_s *inV = skelSF->getVerts();
	const skelWeight_s *inWeights = skelSF->getWeights();
	for(u32 i = 0; i < verts.size(); i++, v++, inV++) {
		const skelWeight_s *w = inWeights + inV->firstWeight;
		v->xyz.clear();
		for(u32 j = 0; j < inV->numWeights; j++, w++) {
			vec3_c p;
			bones[w->boneIndex].mat.transformPoint(w->ofs,p);
			v->xyz += p * w->weight;
		}
	}	
}
void r_surface_c::scaleXYZ(float scale) {
	rVert_c *v = verts.getArray();
	for(u32 i = 0; i < verts.size(); i++, v++) {
		v->xyz *= scale;
	}
	bounds.scaleBB(scale);
}
void r_surface_c::swapYZ() {
	rVert_c *v = verts.getArray();
	for(u32 i = 0; i < verts.size(); i++, v++) {
		float tmp = v->xyz.y;
		v->xyz.y = v->xyz.z;
		v->xyz.z = tmp;
	}
	bounds.swapYZ();
}
void r_surface_c::translateY(float ofs) {
	rVert_c *v = verts.getArray();
	for(u32 i = 0; i < verts.size(); i++, v++) {
		v->xyz.y += ofs;
	}
}
void r_surface_c::multTexCoordsY(float f) {
	rVert_c *v = verts.getArray();
	for(u32 i = 0; i < verts.size(); i++, v++) {
		v->tc.y *= f;
	}
}
void r_surface_c::translateXYZ(const vec3_c &ofs) {
	rVert_c *v = verts.getArray();
	for(u32 i = 0; i < verts.size(); i++, v++) {
		v->xyz += ofs;
	}
	bounds.translate(ofs);
}
void r_surface_c::addPointsToBounds(aabb &out) {
	rVert_c *v = verts.getArray();
	for(u32 i = 0; i < verts.size(); i++, v++) {
		out.addPoint(v->xyz);
	}
}
bool r_surface_c::parseProcSurface(class parser_c &p) {
	int sky = -1;
	if(p.atWord("{")==false) {
		// check for extra 'sky' parameter used in Q4 proc files
		str token = p.getToken();
		if(token.isNumerical() && p.atWord("{")) {
			sky = atoi(token);
		} else {
			g_core->RedWarning("r_surface_c::parseProcSurface: expected '{' to follow \"model\"'s surface in file %s at line %i, found %s\n",
				p.getDebugFileName(),p.getCurrentLineNumber(),p.getToken());
			return true; // error
		}
	}
	this->matName = p.getToken();
	this->mat = g_ms->registerMaterial(this->matName);
	u32 numVerts = p.getInteger();
	u32 numIndices = p.getInteger();
	// read verts
	verts.resize(numVerts);
	rVert_c *v = verts.getArray();
	for(u32 i = 0; i < numVerts; i++, v++) {
		if(p.atWord("(")==false) {
			g_core->RedWarning("r_surface_c::parseProcSurface: expected '(' to follow vertex %i in file %s at line %i, found %s\n",
				i,p.getDebugFileName(),p.getCurrentLineNumber(),p.getToken());
			return true; // error
		}
		p.getFloatMat(v->xyz,3);
		p.getFloatMat(v->tc,2);
		p.getFloatMat(v->normal,3);
		if(p.atWord(")")==false) {
			g_core->RedWarning("r_surface_c::parseProcSurface: expected '(' after vertex %i in file %s at line %i, found %s\n",
				i,p.getDebugFileName(),p.getCurrentLineNumber(),p.getToken());
			return true; // error
		}
	}
	// read triangles
	u16 *indicesu16 = indices.initU16(numIndices);
	for(u32 i = 0; i < numIndices; i++) {
		indicesu16[i] = p.getInteger();
	}
	if(p.atWord("}")==false) {
		g_core->RedWarning("r_surface_c::parseProcSurface: expected closing '}' for \"model\"'s surface block in file %s at line %i, found %s\n",
			p.getDebugFileName(),p.getCurrentLineNumber(),p.getToken());
		return true; // error
	}
	return false; // OK
}
//
//	r_model_c class
//
#include <shared/cmTriSoupOctTree.h>
r_model_c::r_model_c() {
	extraCollOctTree = 0;
}
r_model_c::~r_model_c() {
	if(extraCollOctTree) {
		CMU_FreeTriSoupOctTree(extraCollOctTree);
		extraCollOctTree = 0;
	}
}
void r_model_c::addTriangle(const char *matName, const struct simpleVert_s &v0,
							const struct simpleVert_s &v1, const struct simpleVert_s &v2) {
	r_surface_c *sf = registerSurf(matName);
	sf->addTriangle(v0,v1,v2);
	this->bounds.addPoint(v0.xyz);
	this->bounds.addPoint(v1.xyz);
	this->bounds.addPoint(v2.xyz);
}
void r_model_c::resizeVerts(u32 newNumVerts) {
	if(surfs.size() == 0)
		surfs.resize(1);
	surfs[0].resizeVerts(newNumVerts);
}
void r_model_c::setVert(u32 vertexIndex, const struct simpleVert_s &v) {
	if(surfs.size() == 0)
		surfs.resize(1);
	surfs[0].setVert(vertexIndex,v);
}
void r_model_c::resizeIndices(u32 newNumIndices) {
	if(surfs.size() == 0)
		surfs.resize(1);
	surfs[0].resizeIndices(newNumIndices);
}
void r_model_c::setIndex(u32 indexNum, u32 value) {
	if(surfs.size() == 0)
		surfs.resize(1);
	surfs[0].setIndex(indexNum,value);
}
void r_model_c::scaleXYZ(float scale) {
	r_surface_c *sf = surfs.getArray();
	for(u32 i = 0; i < surfs.size(); i++, sf++) {
		sf->scaleXYZ(scale);
	}
	bounds.scaleBB(scale);
}
void r_model_c::swapYZ() {
	r_surface_c *sf = surfs.getArray();
	for(u32 i = 0; i < surfs.size(); i++, sf++) {
		sf->swapYZ();
	}
	bounds.swapYZ();
}
void r_model_c::translateY(float ofs) {
	r_surface_c *sf = surfs.getArray();
	for(u32 i = 0; i < surfs.size(); i++, sf++) {
		sf->translateY(ofs);
	}
}
void r_model_c::multTexCoordsY(float f) {
	r_surface_c *sf = surfs.getArray();
	for(u32 i = 0; i < surfs.size(); i++, sf++) {
		sf->multTexCoordsY(f);
	}
}
void r_model_c::translateXYZ(const class vec3_c &ofs) {
	r_surface_c *sf = surfs.getArray();
	for(u32 i = 0; i < surfs.size(); i++, sf++) {
		sf->translateXYZ(ofs);
	}
	bounds.translate(ofs);
}
void r_model_c::getCurrentBounds(class aabb &out) {
	r_surface_c *sf = surfs.getArray();
	for(u32 i = 0; i < surfs.size(); i++, sf++) {
		sf->addPointsToBounds(out);
	}
}
void r_model_c::setAllSurfsMaterial(const char *newMatName) {
	r_surface_c *sf = surfs.getArray();
	for(u32 i = 0; i < surfs.size(); i++, sf++) {
		sf->setMaterial(newMatName);
	}
}
void r_model_c::createVBOsAndIBOs() {
	r_surface_c *sf = surfs.getArray();
	for(u32 i = 0; i < surfs.size(); i++, sf++) {
		sf->createIBO();
		sf->createVBO();
	}
}
void r_model_c::addGeometryToColMeshBuilder(class colMeshBuilderAPI_i *out) {
	r_surface_c *sf = surfs.getArray();
	for(u32 i = 0; i < surfs.size(); i++, sf++) {
		sf->addGeometryToColMeshBuilder(out);
	}
}
void r_model_c::initSkelModelInstance(const class skelModelAPI_i *skel) {
	surfs.resize(skel->getNumSurfs());
	r_surface_c *sf = surfs.getArray();
	for(u32 i = 0; i < surfs.size(); i++, sf++) {
		const skelSurfaceAPI_i *inSF = skel->getSurface(i);
		sf->initSkelSurfInstance(inSF);
	}
}
void r_model_c::updateSkelModelInstance(const class skelModelAPI_i *skel, const class boneOrArray_c &bones) {
	r_surface_c *sf = surfs.getArray();
	for(u32 i = 0; i < surfs.size(); i++, sf++) {
		const skelSurfaceAPI_i *inSF = skel->getSurface(i);
		sf->updateSkelSurfInstance(inSF,bones);
	}
}
#include <shared/cmSurface.h>
#include <shared/autoCvar.h>

static aCvar_c r_useTriSoupOctTreesForRayTracing("r_useTriSoupOctTreesForRayTracing","1");
static aCvar_c r_useTriSoupOctTreesForDecalCreation("r_useTriSoupOctTreesForDecalCreation","1");
static aCvar_c r_minOctTreeTrisCount("r_minOctTreeTrisCount","2048");

void r_model_c::ensureExtraTrisoupOctTreeIsBuild() {
	if(extraCollOctTree)
		return;
	if(getTotalTriangleCount() > r_minOctTreeTrisCount.getInt()) {
		cmSurface_c sf;
		addGeometryToColMeshBuilder(&sf);
		extraCollOctTree = CMU_BuildTriSoupOctTree(sf);
	}
}

bool r_model_c::traceRay(class trace_c &tr) {
	if(tr.getTraceBounds().intersect(this->bounds) == false)
		return false;
	if(r_useTriSoupOctTreesForRayTracing.getInt()) {
		ensureExtraTrisoupOctTreeIsBuild();
		if(extraCollOctTree) {
			return extraCollOctTree->traceRay(tr);
		}
	}
	bool hit = false;
	r_surface_c *sf = surfs.getArray();
	for(u32 i = 0; i < surfs.size(); i++, sf++) {
		if(tr.getTraceBounds().intersect(sf->getBB()) == false) {
			continue;
		}
		if(sf->traceRay(tr)) {
			hit = true;
		}
	}
	return hit;
}
bool r_model_c::createDecal(class simpleDecalBatcher_c *out, const class vec3_c &pos,
								   const class vec3_c &normal, float radius, class mtrAPI_i *material) {
	bool hit = false;
	decalProjector_c proj;
	proj.init(pos,normal,radius);
	proj.setMaterial(material);

	// see if we can use extra octree structure to speed up
	// decal creation
	if(r_useTriSoupOctTreesForDecalCreation.getInt()) {
		ensureExtraTrisoupOctTreeIsBuild();
		if(extraCollOctTree) {
			u32 prev = proj.getNumCreatedWindings();
			extraCollOctTree->boxTriangles(proj.getBounds(),&proj);
			// get results
			proj.addResultsToDecalBatcher(out);
			return proj.getNumCreatedWindings() != prev;
		}
	}

	r_surface_c *sf = surfs.getArray();
	for(u32 i = 0; i < surfs.size(); i++, sf++) {
		//if(proj.getBounds().intersect(sf->getBB()) == false) {
		//	continue;
		//}
		if(sf->createDecalInternal(proj)) {
			hit = true;
		}
	}	
	// get results
	proj.addResultsToDecalBatcher(out);
	return hit;
}
r_surface_c *r_model_c::registerSurf(const char *matName) {
	r_surface_c *sf = surfs.getArray();
	for(u32 i = 0; i < surfs.size(); i++, sf++) {
		if(!Q_stricmp(sf->getMatName(),matName)) {
			return sf;
		}
	}	
	sf = &surfs.pushBack();
	sf->setMaterial(matName);
	return sf;
}
void r_model_c::addDrawCalls() {
	r_surface_c *sf = surfs.getArray();
	for(u32 i = 0; i < surfs.size(); i++, sf++) {
		sf->addDrawCall();
	}
}
bool r_model_c::parseProcModel(class parser_c &p) {
	if(p.atWord("{")==false) {
		g_core->RedWarning("r_model_c::parseProcModel: expected '{' to follow \"model\" in file %s at line %i, found %s\n",
			p.getDebugFileName(),p.getCurrentLineNumber(),p.getToken());
		return true; // error
	}
	this->name = p.getToken();
	u32 numSurfs = p.getInteger();
	if(numSurfs) {
		this->bounds.clear();
		this->surfs.resize(numSurfs);
		for(u32 i = 0; i < numSurfs; i++) {
			r_surface_c &sf = surfs[i];
			if(sf.parseProcSurface(p))
				return true; // error occured while parsing the surface
			sf.recalcBB();
			this->bounds.addBox(sf.getBB());
		}
	}
	if(p.atWord("}")==false) {
		g_core->RedWarning("r_model_c::parseProcModel: expected closing '}' for \"model\" block in file %s at line %i, found %s\n",
			p.getDebugFileName(),p.getCurrentLineNumber(),p.getToken());
		return true; // error
	}
	return false;; // OK
}
