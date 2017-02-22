/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef PHYSXCONTROLLER_H
#define PHYSXCONTROLLER_H

#include "physx/PxPhysicsAPI.h"
#include <vector>
#include "HeightMap.h"
#include "Geometry.h"

using namespace physx;

class GeometryArgs
{
public:
	enum GeometryType {
		SPHERE, BOX
	};

	std::vector<PxReal> numericArgs;
	GeometryType type;

	GeometryArgs(const GeometryType type);
	~GeometryArgs();

	void addNumericArg(const PxReal arg);
	void addNumericArgs(const std::vector<PxReal>& args);
};


class PhysXController
{
public:
	static PhysXController& Instance();
	static PxTransform MatToPxTransform(const glm::mat4& matrix);

	PxMaterial* createMaterial(float staticFriction, float dynamicFriction, float restitution);
	PxRigidDynamic* createDynamic(const PxTransform& transform, const PxGeometry& geometry, PxMaterial& material, const PxVec3& velocity, const PxReal angularDamping, PxReal density);
	PxRigidStatic* createStatic(const PxTransform& transform, const PxGeometry& geometry, PxMaterial& material);
	PxRigidStatic* createStatic(const PxTransform& transform);
	PxRigidActor* createEmptyActor(const PxTransform& transform, const bool isStatic);
	PxGeometry* createGeometry(GeometryArgs& geometry);
	PxHeightField* createHeightField(const HeightMap& heightMap);
	PxHeightField* createHeightField(const std::vector<Geometry::TriangleFace>& faces, const float maxHeight);
	void addActorToScene(PxRigidActor* actor);
	void removeActorFromScene(PxRigidActor* actor);
	void addStaticPlane(float nx, float ny, float nz, float distance, PxMaterial* material);

	void update(double dt);
private:
	PxDefaultAllocator gAllocator;
	PxDefaultErrorCallback gErrorCallback;
	PxFoundation* gFoundation;
	PxPhysics* gPhysics;
	PxDefaultCpuDispatcher*	gDispatcher;
	PxScene* gScene;

	PhysXController();
	~PhysXController();

	static PhysXController* instance;
	std::vector<PxMaterial*> materialList;
};

#endif