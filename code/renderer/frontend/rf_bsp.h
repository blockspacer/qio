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
// rf_bsp.h - header for rBspTree_c class
#ifndef __RF_BSP_H__
#define __RF_BSP_H__

#include <shared/typedefs.h>
#include <math/aabb.h>
#include "../rVertexBuffer.h"
#include "../rIndexBuffer.h"
#include <fileFormats/bspFileFormat.h>
#include <shared/bitSet.h>

enum bspSurfaceType_e {
	BSPSF_PLANAR,
	BSPSF_BEZIER,
	BSPSF_TRIANGLES,
};
struct bspTriSurf_s {
	aabb bounds;
	class mtrAPI_i *mat;
	class textureAPI_i *lightmap;
	// indexes to rBspTree_c::verts array (global vertices), for batching
	rIndexBuffer_c absIndexes;
};
struct bspSurf_s {
	bspSurfaceType_e type;
	union {
		struct bspTriSurf_s *sf; // only if type == BSPSF_PLANAR || type == BSPSF_TRIANGLES
		class r_bezierPatch_c *patch; // only if type == BSPSF_BEZIER
	};
	int lastVisCount; // if sf->lastVisCount == bsp->visCounter then a surface is potentialy visible (in PVS)
};
struct bspSurfBatch_s {
	// we can only merge surfaces with the same material and lightmap....
	class mtrAPI_i *mat;
	class textureAPI_i *lightmap;
	// surfaces to merge
	arraySTD_c<bspSurf_s*> sfs;
	// surface bit flags (1 if surfaces indexes are included in current IBO)
	bitSet_c lastVisSet;

	// this index buffer will be recalculated and reuploaded to GPU
	// everytime a new vis cluster is entered.
	rIndexBuffer_c indices;
	// bounds are recalculated as well
	aabb bounds;

	void addSurface(bspSurf_s *nSF) {
		sfs.push_back(nSF);
		indices.addIndexBuffer(nSF->sf->absIndexes);
	}
	void initFromSurface(bspSurf_s *nSF) {
		mat = nSF->sf->mat;
		lightmap = nSF->sf->lightmap;
		addSurface(nSF);
	}
};
struct bspModel_s {
	u32 firstSurf;
	u32 numSurfs;
	aabb bb;
};
struct bspPlane_s {
	float normal[3];
	float dist;

	float distance(const vec3_c &p) const {
		float r = p.x * normal[0] + p.y * normal[1] + p.z * normal[2];
		r -= dist;
		return r;
	}
};
class rBspTree_c {
	byte *fileData;
	const struct q3Header_s *h; // used only while loading
	u32 c_bezierPatches;
	u32 c_flares;
	bitSet_c areaBits;
	visHeader_s *vis;

	rVertexBuffer_c verts;
	arraySTD_c<textureAPI_i*> lightmaps;
	arraySTD_c<bspSurf_s> surfs;
	arraySTD_c<bspSurfBatch_s*> batches;
	arraySTD_c<bspModel_s> models;
	arraySTD_c<bspPlane_s> planes;
	arraySTD_c<q3Node_s> nodes;
	arraySTD_c<q3Leaf_s> leaves;
	arraySTD_c<u32> leafSurfaces;

	// total number of surface indexes in batches (this->batches)
	u32 numBatchSurfIndexes;

	// incremented every time a new vis cluster is entered
	int visCounter;
	int lastCluster;
	int prevNoVis; // last value of rf_bsp_noVis cvar

	void addSurfToBatches(u32 surfNum);
	void createBatches();
	void deleteBatches();
	void createVBOandIBOs();
	void createRenderModelsForBSPInlineModels();

	bool loadLightmaps(u32 lumpNum);
	bool loadPlanes(u32 lumpPlanes);
	bool loadNodesAndLeaves(u32 lumpNodes, u32 lumpLeaves, u32 sizeOfLeaf);
	bool loadSurfs(u32 lumpSurfs, u32 sizeofSurf, u32 lumpIndexes, u32 lumpVerts, u32 lumpMats, u32 sizeofMat);
	bool loadVerts(u32 lumpVerts); // called from loadSurfs / loadSurfsCoD
	bool loadSurfsCoD();
	bool loadModels(u32 modelsLump);
	bool loadLeafIndexes(u32 leafSurfsLump);
	bool loadVisibility(u32 visLump);

	void addBSPSurfaceDrawCall(u32 sfNum);

	bool traceSurfaceRay(u32 surfNum, class trace_c &out);
	void traceNodeRay(int nodeNum, class trace_c &out);

	int pointInLeaf(const vec3_c &pos) const;
	int pointInCluster(const vec3_c &pos) const;
	bool isClusterVisible(int visCluster, int testCluster) const;
public:
	rBspTree_c();
	~rBspTree_c();

	bool load(const char *fname);
	void clear();

	void updateVisibility();
	void addDrawCalls();
	void addModelDrawCalls(u32 inlineModelNum);

	void setWorldAreaBits(const byte *bytes, u32 numBytes);

	void traceRay(class trace_c &out);
	bool traceRayInlineModel(u32 inlineModelnum, class trace_c &out);
};

rBspTree_c *RF_LoadBSP(const char *fname);

#endif // __RF_BSP_H__

