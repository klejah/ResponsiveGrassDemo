/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "PhysXController.h"
#include <iostream>
#include <algorithm>

#define USE_PVD true

PhysXController* PhysXController::instance = 0;

PhysXController& PhysXController::Instance()
{
	if (instance != 0)
	{
		return *instance;
	}
	instance = new PhysXController();
	return *instance;
}

PxTransform PhysXController::MatToPxTransform(const glm::mat4& matrix)
{
	physx::PxMat44 m((float *)&matrix);
	return physx::PxTransform(m);
}

PxMaterial* PhysXController::createMaterial(float staticFriction, float dynamicFriction, float restitution)
{
	PxReal sF = (PxReal)staticFriction;
	PxReal dF = (PxReal)dynamicFriction;
	PxReal r = (PxReal)restitution;
	//TODO perhaps hash it
	for (unsigned int i = 0; i < materialList.size(); i++)
	{
		if (materialList[i]->getStaticFriction() == sF && materialList[i]->getDynamicFriction() == dF && materialList[i]->getRestitution() == r)
		{
			return materialList[i];
		}
	}
	PxMaterial* ret = gPhysics->createMaterial(sF, dF, r);
	materialList.push_back(ret);
	return ret;
}

PxRigidDynamic* PhysXController::createDynamic(const PxTransform& transform, const PxGeometry& geometry, PxMaterial& material, const PxVec3& velocity, const PxReal angularDamping, PxReal density)
{
	PxRigidDynamic* dynamic = PxCreateDynamic(*gPhysics, transform, geometry, material, density);
	dynamic->setAngularDamping(angularDamping);
	dynamic->setLinearVelocity(velocity);
	return dynamic;
}

PxRigidStatic* PhysXController::createStatic(const PxTransform& transform, const PxGeometry& geometry, PxMaterial& material)
{
	PxRigidStatic* rigidStatic = PxCreateStatic(*gPhysics, transform, geometry, material);
	return rigidStatic;
}

PxRigidStatic* PhysXController::createStatic(const PxTransform& transform)
{
	return gPhysics->createRigidStatic(transform);
}

PxRigidActor* PhysXController::createEmptyActor(const PxTransform& transform, const bool isStatic)
{
	if (isStatic)
		return gPhysics->createRigidStatic(transform);
	else
		return gPhysics->createRigidDynamic(transform);
}

void PhysXController::addActorToScene(PxRigidActor* actor)
{
	if (actor != 0)
	{
		gScene->addActor(*actor);
	}
}

void PhysXController::removeActorFromScene(PxRigidActor* actor)
{
	if (actor != 0)
	{
		gScene->removeActor(*actor);
	}
}

PxGeometry* PhysXController::createGeometry(GeometryArgs& geometry)
{
	switch (geometry.type)
	{
	case GeometryArgs::GeometryType::SPHERE:
		if (geometry.numericArgs.size() >= 1)
		{
			return new PxSphereGeometry(geometry.numericArgs[0]);
		}
		break;
	case GeometryArgs::GeometryType::BOX:
		if (geometry.numericArgs.size() >= 3)
		{
			return new PxBoxGeometry(geometry.numericArgs[0], geometry.numericArgs[1], geometry.numericArgs[2]);
		}
		break;
	default:
		break;
	}
	std::cout << "ERROR: PhysXController::createGeometry - Unknown Type or wrong arguments." << std::endl;
	return new PxSphereGeometry();
}

PxHeightField* PhysXController::createHeightField(const HeightMap& heightMap)
{
	PxHeightFieldDesc desc;
	PxU32 x = heightMap.Height();
	PxU32 y = heightMap.Width();
	desc.nbColumns = x;
	desc.nbRows = y;
	PxU32* sampleData = new PxU32[x * y];

	PxI16* pixels = new PxI16[x * y];
	heightMap.bind(0);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_SHORT, pixels);
	glBindTexture(GL_TEXTURE_2D, 0);

	for (unsigned int i = 0; i < y; i++)
	{
		for (unsigned int j = 0; j < x; j++)
		{
			PxU32 index = i * x + j;
			PxU32 pIndex = j * x + i;
			PxHeightFieldSample& currentSample = (PxHeightFieldSample&)(sampleData[index]);
			currentSample.height = pixels[pIndex];
			currentSample.materialIndex0 = 0;
			currentSample.materialIndex1 = 0;
			currentSample.clearTessFlag();
		}
	}
	delete pixels;

	desc.samples.data = sampleData;
	desc.samples.stride = sizeof(PxU32);

	PxHeightField* hf = gPhysics->createHeightField(desc);
	delete sampleData;

	return hf;
}

PxHeightField* PhysXController::createHeightField(const std::vector<Geometry::TriangleFace>& faces, const float maxHeight)
{
	std::vector<glm::vec3> vertices;
	for each (Geometry::TriangleFace f in faces)
	{
		for (unsigned int i = 0; i < 3; i++)
		{
			auto v = f.vertices[i].position;
			bool found = false;
			for each (glm::vec3 vert in vertices)
			{
				glm::vec2 rv = glm::vec2(glm::round(v.x), glm::round(v.z));
				glm::vec2 rvert = glm::vec2(glm::round(vert.x), glm::round(vert.z));
				glm::vec2 vec = rv - rvert;
				if (glm::dot(vec, vec) < 0.1f)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				vertices.push_back(v);
			}
		}
	}
	std::cout << "Vertices: " << vertices.size() << std::endl;

	struct {
		bool operator()(glm::vec3& a, glm::vec3& b)
		{
			if (glm::round(a.x) < glm::round(b.x))
			{
				return true;
			}
			if (glm::round(a.x) == glm::round(b.x) && glm::round(a.z) < glm::round(b.z))
			{
				return true;
			}
			return false;
		}
	} sortingFunc;

	std::sort(vertices.begin(), vertices.end(), sortingFunc);

	PxU32 x = 0;
	PxU32 y = 1;
	float fX = glm::round(vertices[0].x);
	float curX = glm::round(vertices[0].x);
	for (unsigned int i = 0; i < vertices.size(); i++)
	{
		glm::vec3 v = vertices[i];
		if (glm::round(v.x) == fX)
		{
			x++;
		}
		else if (glm::round(v.x) != curX)
		{
			curX = glm::round(v.x);
			y++;
		}
	}
	std::cout << "Nr Row " << y << " nr col " << x << std::endl;

	PxHeightFieldDesc desc;
	desc.nbColumns = x;
	desc.nbRows = y;

	PxI16* heights = new PxI16[x * y];
	unsigned int curx = 0;
	unsigned int cury = -1;
	for (unsigned int i = 0; i < vertices.size(); i++)
	{
		glm::vec3 v = vertices[i];
		if (glm::round(v.x) == curX)
		{
			curx++;
		}
		else
		{
			curX = glm::round(v.x);
			curx = 0;
			cury++;
		}
		heights[cury * x + curx] = (PxI16)((v.y / maxHeight) * (float)SHRT_MAX);
	}

	PxU32* sampleData = new PxU32[x * y];

	for (unsigned int i = 0; i < y; i++)
	{
		for (unsigned int j = 0; j < x; j++)
		{
			PxU32 index = i * x + j;
			PxHeightFieldSample& currentSample = (PxHeightFieldSample&)(sampleData[index]);
			currentSample.height = heights[index];
			currentSample.materialIndex0 = 0;
			currentSample.materialIndex1 = 0;
			currentSample.clearTessFlag();
		}
	}
	delete heights;

	desc.samples.data = sampleData;
	desc.samples.stride = sizeof(PxU32);

	PxHeightField* hf = gPhysics->createHeightField(desc);
	delete sampleData;

	return hf;
}

void PhysXController::addStaticPlane(float nx, float ny, float nz, float distance, PxMaterial* material)
{
	PxRigidStatic* plane = PxCreatePlane(*gPhysics, PxPlane(nx, ny, nz, distance), *material);
	gScene->addActor(*plane);
}

PhysXController::PhysXController()
{
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
	PxProfileZoneManager* profileZoneManager = &PxProfileZoneManager::createProfileZoneManager(gFoundation);
	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, profileZoneManager);

	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.bounceThresholdVelocity = 0.2f * sceneDesc.gravity.magnitude();
	gDispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = gDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	gScene = gPhysics->createScene(sceneDesc);

#ifdef _DEBUG
	if (USE_PVD)
	{
		profileZoneManager->addProfileZone(PxProfileZone::createProfileZone(gFoundation, "GraphicsTestV2"));
		PxInitExtensions(*gPhysics);

		PxVisualDebuggerConnectionManager* pvd = gPhysics->getPvdConnectionManager();
		if (!pvd)
			return;
		PxVisualDebuggerConnectionFlags theConnectionFlags(PxVisualDebuggerConnectionFlag::eDEBUG |
			PxVisualDebuggerConnectionFlag::ePROFILE | PxVisualDebuggerConnectionFlag::eMEMORY);
		auto con = PxVisualDebuggerExt::createConnection(pvd, "127.0.0.1", 5425, 10000, theConnectionFlags);
		std::cout << "PVD is connected? " << std::to_string(pvd->isConnected())  << std::endl;
		gPhysics->getVisualDebugger()->setVisualizeConstraints(true);
		gPhysics->getVisualDebugger()->setVisualDebuggerFlag(PxVisualDebuggerFlag::eTRANSMIT_CONTACTS, true);
		gPhysics->getVisualDebugger()->setVisualDebuggerFlag(PxVisualDebuggerFlag::eTRANSMIT_SCENEQUERIES, true);
	}
#endif
}

PhysXController::~PhysXController()
{
	for each(PxMaterial* m in materialList)
	{
		m->release();
	}

	if (gDispatcher != 0)
	{
		gDispatcher->release();
	}

	if (gPhysics != 0)
	{
		gPhysics->release();
	}

	if (gScene != 0)
	{
		gScene->release();
	}

	if (gFoundation != 0)
	{
		gFoundation->release();
	}
}

void PhysXController::update(double dt)
{
	gScene->simulate((PxReal)dt);
	gScene->fetchResults(true); //true = blocking
}

///////////////////////////////////////
// Geometry Args //////////////////////
///////////////////////////////////////

GeometryArgs::GeometryArgs(GeometryType _type) : type(_type)
{

}

GeometryArgs::~GeometryArgs()
{

}

void GeometryArgs::addNumericArg(const PxReal arg)
{
	numericArgs.push_back(arg);
}

void GeometryArgs::addNumericArgs(const std::vector<PxReal>& args)
{
	numericArgs.insert(args.end(), numericArgs.begin(), numericArgs.end());
}