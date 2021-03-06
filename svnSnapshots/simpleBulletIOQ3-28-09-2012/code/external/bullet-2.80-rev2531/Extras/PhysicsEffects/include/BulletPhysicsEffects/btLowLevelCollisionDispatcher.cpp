/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2007 Erwin Coumans  http://bulletphysics.com

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/


#include "btLowLevelCollisionDispatcher.h"
#include "BulletCollision/BroadphaseCollision/btOverlappingPairCache.h"
#include "BulletCollision/CollisionDispatch/btEmptyCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/CollisionShapes/btCollisionShape.h"
#include "LinearMath/btQuickprof.h"
#include "btLowLevelBroadphase.h"
#include "btLowLevelData.h"
#include "stdio.h"//??

#include <physics_effects/low_level/pfx_low_level_include.h>
#include <physics_effects/util/pfx_util_include.h>

using namespace sce::PhysicsEffects;

btLowLevelCollisionDispatcher::btLowLevelCollisionDispatcher(btLowLevelData* lowLevelData, btCollisionConfiguration* collisionConfiguration, int maxNumManifolds)
:btCollisionDispatcher(collisionConfiguration),
m_lowLevelData(lowLevelData)
{
	m_manifoldArray.resize(maxNumManifolds);
	m_algorithms.resize(maxNumManifolds);
}


bool	btLowLevelCollisionDispatcher::supportsDispatchPairOnSpu(int proxyType0,int proxyType1)
{
	bool supported0 = (
		(proxyType0 == BOX_SHAPE_PROXYTYPE) ||
		(proxyType0 == TRIANGLE_SHAPE_PROXYTYPE) ||
		(proxyType0 == SPHERE_SHAPE_PROXYTYPE) ||
		(proxyType0 == CAPSULE_SHAPE_PROXYTYPE) ||
		(proxyType0 == CYLINDER_SHAPE_PROXYTYPE) ||
//		(proxyType0 == CONE_SHAPE_PROXYTYPE) ||
		(proxyType0 == TRIANGLE_MESH_SHAPE_PROXYTYPE) ||
		(proxyType0 == CONVEX_HULL_SHAPE_PROXYTYPE)||
		(proxyType0 == STATIC_PLANE_PROXYTYPE)||
		(proxyType0 == COMPOUND_SHAPE_PROXYTYPE)
		);

	bool supported1 = (
		(proxyType1 == BOX_SHAPE_PROXYTYPE) ||
		(proxyType1 == TRIANGLE_SHAPE_PROXYTYPE) ||
		(proxyType1 == SPHERE_SHAPE_PROXYTYPE) ||
		(proxyType1 == CAPSULE_SHAPE_PROXYTYPE) ||
		(proxyType1 == CYLINDER_SHAPE_PROXYTYPE) ||
//		(proxyType1 == CONE_SHAPE_PROXYTYPE) ||
		(proxyType1 == TRIANGLE_MESH_SHAPE_PROXYTYPE) ||
		(proxyType1 == CONVEX_HULL_SHAPE_PROXYTYPE) ||
		(proxyType1 == STATIC_PLANE_PROXYTYPE) ||
		(proxyType1 == COMPOUND_SHAPE_PROXYTYPE)
		);

	
	return supported0 && supported1;
}



btLowLevelCollisionDispatcher::~btLowLevelCollisionDispatcher()
{	
}




void btLowLevelCollisionDispatcher::collision()
{
	unsigned int numCurrentPairs = m_lowLevelData->getNumCurrentPairs();//->m_numPairs[pairSwap];
	PfxBroadphasePair *currentPairs = m_lowLevelData->getCurrentPairs();//pairsBuff[pairSwap];
	//J 衝突検出
	//E Detect collisions
	{
		PfxDetectCollisionParam param;
		param.contactPairs = currentPairs;
		param.numContactPairs = numCurrentPairs;
		param.offsetContactManifolds = m_lowLevelData->m_contacts;
		param.offsetRigidStates = m_lowLevelData->m_states;
		param.offsetCollidables = m_lowLevelData->m_collidables;
		param.numRigidBodies = m_lowLevelData->m_numRigidBodies;

		int ret = pfxDetectCollision(param);
		if(ret != SCE_PFX_OK) SCE_PFX_PRINTF("pfxDetectCollision failed %d\n",ret);
	}

	//J リフレッシュ
	//E Refresh contacts
	{
		PfxRefreshContactsParam param;
		param.contactPairs = currentPairs;
		param.numContactPairs = numCurrentPairs;
		param.offsetContactManifolds = m_lowLevelData->m_contacts;
		param.offsetRigidStates = m_lowLevelData->m_states;
		param.numRigidBodies = m_lowLevelData->m_numRigidBodies;

		int ret = pfxRefreshContacts(param);
		if(ret != SCE_PFX_OK) SCE_PFX_PRINTF("pfxRefreshContacts failed %d\n",ret);
	}

}

void btLowLevelCollisionDispatcher::releaseManifold(btPersistentManifold* manifold)
{
	int findIndex = manifold->m_index1a;
	btAssert(findIndex < m_manifoldsPtr.size());
	m_manifoldsPtr.swap(findIndex,m_manifoldsPtr.size()-1);
	m_manifoldsPtr[findIndex]->m_index1a = findIndex;
	m_manifoldsPtr.pop_back();
}

void	btLowLevelCollisionDispatcher::dispatchAllCollisionPairs(btOverlappingPairCache* pairCache,const btDispatcherInfo& dispatchInfo, btDispatcher* dispatcher) 
{


	if (1)
	{
		//use low level contact generation
		//int numTotalPairs = pairCache->getNumOverlappingPairs();
		//btBroadphasePair* pairPtr = pairCache->getOverlappingPairArrayPtr();

		//fetch pairs directly from m_llbp
		int numPairs = m_lowLevelData->getNumCurrentPairs();
		PfxBroadphasePair* pairs = m_lowLevelData->getCurrentPairs();
		for (int i=0;i<numPairs;i++)
		{
			PfxBroadphasePair& pair = pairs[i];

			int uidA = pfxGetObjectIdA(pair);
			int uidB = pfxGetObjectIdB(pair);
			int maskA = pfxGetMotionMaskA(pair);
			int maskB = pfxGetMotionMaskB(pair);
			int flag = pfxGetBroadphaseFlag(pair);
			int active = pfxGetActive(pair);
			int contactId = pfxGetContactId(pair);
			//printf("contactId = %d\n", contactId);
			


		}

		collision();

		//copy the results back into btPersistentManifold structures. How to do the mapping between PfxBroadphasePair and btBroadphasePair?
		for (int i=0;i<pairCache->getNumOverlappingPairs();i++)
		{
			btBroadphasePair& btpair = pairCache->getOverlappingPairArrayPtr()[i];
			int contactId = btpair.m_internalTmpValue;
			PfxContactManifold& pfxcontacts = m_lowLevelData->m_contacts[contactId];
			btManifoldArray btmanifolds;
			btCollisionObject* body0 = (btCollisionObject*)btpair.m_pProxy0->m_clientObject;
			btCollisionObject* body1 = (btCollisionObject*)btpair.m_pProxy1->m_clientObject;


			if (!btpair.m_algorithm)
			{
				btPersistentManifold* mm = &m_manifoldArray[contactId];//getNewManifold(btpair.m_pProxy0,btpair.m_pProxy1);

			
				//optional relative contact breaking threshold, turned on by default (use setDispatcherFlags to switch off feature for improved performance)
				
				btScalar contactBreakingThreshold =  (m_dispatcherFlags & btCollisionDispatcher::CD_USE_RELATIVE_CONTACT_BREAKING_THRESHOLD) ? 
					btMin(body0->getCollisionShape()->getContactBreakingThreshold(gContactBreakingThreshold) , body1->getCollisionShape()->getContactBreakingThreshold(gContactBreakingThreshold))
					: gContactBreakingThreshold ;

				btScalar contactProcessingThreshold = btMin(body0->getContactProcessingThreshold(),body1->getContactProcessingThreshold());

				btPersistentManifold* newManifold = new (mm) btPersistentManifold(body0,body1,0,contactBreakingThreshold,contactProcessingThreshold);

				newManifold->m_index1a = m_manifoldsPtr.size();
				m_manifoldsPtr.push_back(newManifold);
				char* mem = (char*) &m_algorithms[contactId];

				btpair.m_algorithm = new(mem)btLowLevelCollisionAlgorithm(newManifold, this);
			}
			btpair.m_algorithm->getAllContactManifolds(btmanifolds);
			if (btmanifolds.size()==1)
			{
				btPersistentManifold* manifold = btmanifolds[0];
				
				btManifoldResult contactPointResult(body0,body1);
				contactPointResult.setPersistentManifold(manifold);
				
				for (int n=0;n<pfxcontacts.getNumContacts();n++)
				{
					sce::PhysicsEffects::PfxContactPoint& pxpt = pfxcontacts.getContactPoint(n);
					
					
					btVector3 normalOnBInWorld(pxpt.m_constraintRow->m_normal[0],pxpt.m_constraintRow->m_normal[1],pxpt.m_constraintRow->m_normal[2]);
					btVector3 localPointB(pxpt.m_localPointB[0],pxpt.m_localPointB[1],pxpt.m_localPointB[2]);
					btVector3 pointInWorld = body1->getWorldTransform()*localPointB;
					btScalar depth = pxpt.m_distance;
					contactPointResult.addContactPoint(normalOnBInWorld,pointInWorld,depth);
					
				}
				manifold->refreshContactPoints(body0->getWorldTransform(),body1->getWorldTransform());
			}
			
			

		}
		

		//

	} 
	else
	{
					
		//pairCache->getNumOverlappingPairs();



		///PPU fallback
		///!Need to make sure to clear all 'algorithms' when switching between SPU and PPU
		btCollisionDispatcher::dispatchAllCollisionPairs(pairCache,dispatchInfo,dispatcher);
	}
}
