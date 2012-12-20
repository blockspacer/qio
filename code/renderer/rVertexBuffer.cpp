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
// rVertexBuffer.cpp
#include "rVertexBuffer.h"
#include "rIndexBuffer.h"

void rVertexBuffer_c::calcEnvironmentTexCoords(const vec3_c &viewerOrigin) {
	rVert_c *v = this->getArray();
	for(u32 i = 0; i < this->size(); i++, v++) {
		v->calcEnvironmentTexCoords(viewerOrigin);
	}
}
void rVertexBuffer_c::calcEnvironmentTexCoordsForReferencedVertices(const class rIndexBuffer_c &ibo, const class vec3_c &viewerOrigin) {
	static arraySTD_c<byte> bVertexCalculated;
	if(this->size() > bVertexCalculated.size()) {
		bVertexCalculated.resize(this->size());
	}
	memset(bVertexCalculated.getArray(),0,this->size());
	for(u32 i = 0; i < ibo.getNumIndices(); i++) {
		u32 index = ibo[i];
		if(bVertexCalculated[index] == 0) {
			this->data[index].calcEnvironmentTexCoords(viewerOrigin);
			bVertexCalculated[index] = 1;
		}
	}
}

void rVertexBuffer_c::setVertexColorsToConstValue(byte val) {
	rVert_c *v = this->getArray();
	for(u32 i = 0; i < this->size(); i++, v++) {
		v->color[0] = v->color[1] = v->color[2] = v->color[3] = val;
	}
}
void rVertexBuffer_c::setVertexColorsToConstValues(byte *rgbVals) {
	rVert_c *v = this->getArray();
	for(u32 i = 0; i < this->size(); i++, v++) {
		v->color[0] = rgbVals[0];
		v->color[1] = rgbVals[1];
		v->color[2] = rgbVals[2];
	}
}
void rVertexBuffer_c::setVertexAlphaToConstValue(byte val) {
	rVert_c *v = this->getArray();
	for(u32 i = 0; i < this->size(); i++, v++) {
		v->color[3] = val;
	}
}