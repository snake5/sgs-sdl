
#include "bullet/src/BulletCollision/BroadphaseCollision/btAxisSweep3.cpp"
#include "bullet/src/BulletCollision/BroadphaseCollision/btBroadphaseProxy.cpp"
#include "bullet/src/BulletCollision/BroadphaseCollision/btCollisionAlgorithm.cpp"
#include "bullet/src/BulletCollision/BroadphaseCollision/btDbvt.cpp"
#include "bullet/src/BulletCollision/BroadphaseCollision/btDbvtBroadphase.cpp"
#include "bullet/src/BulletCollision/BroadphaseCollision/btDispatcher.cpp"
#include "bullet/src/BulletCollision/BroadphaseCollision/btMultiSapBroadphase.cpp"
#include "bullet/src/BulletCollision/BroadphaseCollision/btOverlappingPairCache.cpp"
#include "bullet/src/BulletCollision/BroadphaseCollision/btQuantizedBvh.cpp"
#include "bullet/src/BulletCollision/BroadphaseCollision/btSimpleBroadphase.cpp"

#include "bullet/src/BulletCollision/CollisionDispatch/btActivatingCollisionAlgorithm.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btBox2dBox2dCollisionAlgorithm.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btBoxBoxCollisionAlgorithm.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btBoxBoxDetector.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btCollisionDispatcher.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btCollisionObject.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btCollisionWorld.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btCompoundCollisionAlgorithm.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btCompoundCompoundCollisionAlgorithm.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btConvex2dConvex2dAlgorithm.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btConvexConcaveCollisionAlgorithm.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btConvexConvexAlgorithm.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btConvexPlaneCollisionAlgorithm.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btEmptyCollisionAlgorithm.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btGhostObject.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btHashedSimplePairCache.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btInternalEdgeUtility.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btManifoldResult.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btSimulationIslandManager.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btSphereBoxCollisionAlgorithm.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/btUnionFind.cpp"
#include "bullet/src/BulletCollision/CollisionDispatch/SphereTriangleDetector.cpp"

#include "bullet/src/BulletCollision/CollisionShapes/btBox2dShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btBoxShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btBvhTriangleMeshShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btCapsuleShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btCollisionShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btCompoundShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btConcaveShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btConeShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btConvex2dShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btConvexHullShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btConvexInternalShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btConvexPointCloudShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btConvexPolyhedron.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btConvexShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btConvexTriangleMeshShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btCylinderShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btEmptyShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btMinkowskiSumShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btMultimaterialTriangleMeshShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btMultiSphereShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btOptimizedBvh.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btPolyhedralConvexShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btShapeHull.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btSphereShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btStaticPlaneShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btStridingMeshInterface.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btTetrahedronShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btTriangleBuffer.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btTriangleCallback.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btTriangleIndexVertexArray.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btTriangleIndexVertexMaterialArray.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btTriangleMesh.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btTriangleMeshShape.cpp"
#include "bullet/src/BulletCollision/CollisionShapes/btUniformScalingShape.cpp"

#if 0
#include "bullet/src/BulletCollision/Gimpact/btContactProcessing.cpp"
#include "bullet/src/BulletCollision/Gimpact/btGenericPoolAllocator.cpp"
#include "bullet/src/BulletCollision/Gimpact/btGImpactBvh.cpp"
#include "bullet/src/BulletCollision/Gimpact/btGImpactCollisionAlgorithm.cpp"
#include "bullet/src/BulletCollision/Gimpact/btGImpactQuantizedBvh.cpp"
#include "bullet/src/BulletCollision/Gimpact/btGImpactShape.cpp"
#include "bullet/src/BulletCollision/Gimpact/btTriangleShapeEx.cpp"
#include "bullet/src/BulletCollision/Gimpact/gim_box_set.cpp"
#undef TEST_CROSS_EDGE_BOX_MCR
#include "bullet/src/BulletCollision/Gimpact/gim_contact.cpp"
#include "bullet/src/BulletCollision/Gimpact/gim_memory.cpp"
#include "bullet/src/BulletCollision/Gimpact/gim_tri_collision.cpp"
#endif

#include "bullet/src/BulletCollision/NarrowPhaseCollision/btContinuousConvexCollision.cpp"
#include "bullet/src/BulletCollision/NarrowPhaseCollision/btConvexCast.cpp"
#undef MAX_ITERATIONS
#include "bullet/src/BulletCollision/NarrowPhaseCollision/btGjkConvexCast.cpp"
#include "bullet/src/BulletCollision/NarrowPhaseCollision/btGjkEpa2.cpp"
#include "bullet/src/BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.cpp"
#include "bullet/src/BulletCollision/NarrowPhaseCollision/btGjkPairDetector.cpp"
#include "bullet/src/BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.cpp"
#include "bullet/src/BulletCollision/NarrowPhaseCollision/btPersistentManifold.cpp"
#define IsAlmostZero IsAlmostZero2
#include "bullet/src/BulletCollision/NarrowPhaseCollision/btPolyhedralContactClipping.cpp"
#include "bullet/src/BulletCollision/NarrowPhaseCollision/btRaycastCallback.cpp"
#include "bullet/src/BulletCollision/NarrowPhaseCollision/btSubSimplexConvexCast.cpp"
#include "bullet/src/BulletCollision/NarrowPhaseCollision/btVoronoiSimplexSolver.cpp"
