#pragma once
#include <unordered_map>
#include <nodePath.h>
#include <collisionCapsule.h>
#include <collisionHandlerQueue.h>
#include <collisionRay.h>
#include <collisionSphere.h>
#include <collisionPlane.h>
#include <pandaFramework.h>
#include <mouseWatcher.h>
#include <clockObject.h>
#include <transparencyAttrib.h>
#define DegToRad(deg) (deg * 3.14159 / 180.0)
#define RadToDeg(rad) (rad * 180.0 / 3.14159)

LVector3f MultVecs (const LVecBase3f& vec1, const LVecBase3f& vec2); 

class Tools {
	WindowFramework* window;

	//Collisions
	PT(CollisionHandlerQueue) toolHandl = new CollisionHandlerQueue();
	PT(CollisionRay) toolRay = new CollisionRay();
	PT(CollisionNode) plnd = new CollisionNode("plnode"); // Plane node
	NodePath plane = NodePath ("collplane"); // Plane for arrows
	CollisionTraverser toolTrav = CollisionTraverser ("arrtrav");

	// Previous ray intersection position
	LPoint3f prevPos = LPoint3f(0.0, 0.0, 0.0);

	// Global/local orentation
	bool glob = true;

	// Objects
	short int tool = 0; // Tool: arrows (moving), circles (rotating), sizers (resizing).
	short int toolnum = -1;
	NodePath arrowroot = NodePath("arrowroot");
	NodePath circleroot = NodePath("circleroot");
	NodePath sizerroot = NodePath("sizerroot");

	void AddShapeToArrow (NodePath& obj); // Adds collision shape to arrow when it is created.
	inline void AddShapeToCircle (NodePath& obj);  // Adds collision shape to circle (toroid) when it is created.
	void AddShapeToSizer (NodePath& obj); // Adds collision shape to sizer when it is created.
	void AddShapeToSiserS (NodePath& sizer); // Adds collision shape to spherical sizer when it is created.
	void processArrows (const NodePath& camera); // Set arrows' scale and their rendering order.
	void processCircles(const NodePath& camera); // Set circles' scale and their rendering order.
	void processSizers (const NodePath& camera); // Set sizers' scale and their rendering order.

	void dragObj (NodePath& pcdObj, const NodePath& rcroot, const MouseWatcher* mWatcher, bool first = false); // Drag object with arrows.
	inline void postProcArrows (); // Post-processing arrows.
	void rotObj(NodePath& pcdObj, const NodePath& rcroot, const MouseWatcher* mWatcher, bool first = false); // Rotate object with circles.
	inline void postProcCircles (const NodePath& pcdObj, const NodePath& rcroot); // Rotate circles after local transformation.
	void resizeObj (NodePath& pcdObj, const NodePath& rcroot, const MouseWatcher* mWatcher, bool first = false); // Resize object with sizers.
	inline void postProcSizers (); // Rotate circles after local transformation.
	inline void SetPlaneRot (const NodePath& arrow, const NodePath& camera); // Rotate a plane that is placed to arrow position to make it look to the camera.
	inline void crlSetPlaneRot (const NodePath& camera); // Same thing as SetPlaneRot but for circles.
public:
	Tools ();
	void Init (WindowFramework* win, NodePath& camera); // Initialize tools.
	void processTool (NodePath& camera, NodePath& rcroot); // Make current tool look COOL! Process them every frame.
	void processObj(NodePath& pcdObj, std::unordered_map<void*, NodePath>& pcdObjs, const NodePath& rcroot, const MouseWatcher* mWatcher, bool first = false); // Operate(drag, resize or rotate) object with current tool.
	void postProcessObj (const NodePath& pcdObj, const NodePath& rcroot);
	void changeTool(short int num = 0, bool global = true, bool reShow = false, const LPoint3f& pos = LPoint3f(), const LVector3f& rot = LVector3f(), const NodePath& camera = NodePath(), const NodePath& rcroot = NodePath()); // Change current tool to another.
	void setToolnum (short int num = -1);
	void setupTool (NodePath& entry, NodePath& pcdObj, const NodePath& rcroot, const NodePath& camera, const MouseWatcher* mWatcher); // Prepare current tool for operations.
	void hideTools ();
	short int getTool ();
	short int getToolnum(); // Get number of current tool.
	NodePath& toolroot (); // Get root of tools - their parent node.
};
