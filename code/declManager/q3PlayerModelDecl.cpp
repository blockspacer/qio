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
// q3PlayerModelDecl.cpp - Quake3 three-parts player model system
#include "q3PlayerModelDecl.h"
#include <api/vfsAPI.h>
#include <api/modelLoaderDLLAPI.h>
#include <api/kfModelAPI.h>
#include <shared/parser.h>
#include <shared/quake3AnimationConfig.h>
#include <shared/singleAnimLerp.h>
#include <shared/tagOr.h>
#include <math/matrix.h>

u32 q3PlayerModelDecl_c::getNumTotalSurfaces() const {
	u32 ret = 0;
	if(legsModel) {
		ret += legsModel->getNumSurfaces();
	}
	if(torsoModel) {
		ret += torsoModel->getNumSurfaces();
	}
	if(headModel) {
		ret += headModel->getNumSurfaces();
	}
	return ret;
}
const struct q3AnimDef_s *q3PlayerModelDecl_c::getAnimCFGForIndex(u32 localAnimIndex) const {
	if(cfg == 0)
		return 0;
	return cfg->getAnimCFGForIndex(localAnimIndex);
}
int q3PlayerModelDecl_c::getTagNumForName(const char *boneName) const {
	int localIndex = legsModel->getTagIndexForName(boneName);
	if(localIndex != -1) {
		return localIndex;
	}
	u32 numLegsTags = legsModel->getNumTags();
	localIndex = torsoModel->getTagIndexForName(boneName);
	if(localIndex != -1) {
		return numLegsTags + localIndex;
	}
	return -1;
}
void q3PlayerModelDecl_c::getTagOrientation(int tagNum, const struct singleAnimLerp_s &legs, const struct singleAnimLerp_s &torso, class matrix_c &out) const {
	if(this->isLegsTag(tagNum)) {

	} else {
		// we need to get torso orientation first, which is attached to legs tag	
		// attach torso model to legs model tag
		const tagOr_c *torsoTag = legsModel->getTagOrientation("tag_torso",legs.from);
		matrix_c torsoTagMat;
		torsoTag->toMatrix(torsoTagMat);
		u32 localTagNum = tagNum - legsModel->getNumTags();
		const tagOr_c *tag = torsoModel->getTagOrientation(localTagNum,torsoModel->fixFrameNum(torso.from));
		matrix_c tagMatrix;
		tag->toMatrix(tagMatrix);
		out = torsoTagMat * tagMatrix;
	}
}
bool q3PlayerModelDecl_c::isLegsTag(int tagNum) const {
	if(tagNum < 0)
		return false; // it's not a valid tag num
	if(tagNum < legsModel->getNumTags())
		return true;
	return false;
}
bool q3PlayerModelDecl_c::loadQ3PlayerDecl() {
	legsModelPath = "models/players/";
	legsModelPath.append(this->name);
	legsModelPath.append("/lower.md3");
	legsModel = g_modelLoader->loadKeyFramedModelFile(legsModelPath);

	torsoModelPath = "models/players/";
	torsoModelPath.append(this->name);
	torsoModelPath.append("/upper.md3");
	torsoModel = g_modelLoader->loadKeyFramedModelFile(torsoModelPath);

	headModelPath = "models/players/";
	headModelPath.append(this->name);
	headModelPath.append("/head.md3");
	headModel = g_modelLoader->loadKeyFramedModelFile(headModelPath);

	str configFilePath = "models/players/";
	configFilePath.append(this->name);
	configFilePath.append("/animation.cfg");
	cfg = new q3AnimCfg_c;
	if(cfg->parse(configFilePath)) {
		delete cfg;
		cfg = 0;
	}

	if(legsModel && torsoModel && headModel && cfg) {
		valid = true;
	}

	return false;
}