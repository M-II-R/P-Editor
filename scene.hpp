#pragma once
#include <atomic>
#include <cmath>
#include <unordered_map>

#include <pandaFramework.h>
#include <pandaSystem.h>
#include <modelNode.h>
#include <modelRoot.h>

#include <mouseButton.h>
#include <mouseWatcher.h>
#include <clockObject.h>
#include <occluderNode.h>
#include <lens.h>
#include <camera.h>
//#include <displayRegion.h>
//#include <aa_luse.h>

#include <asyncTask.h>
#include <collisionHandlerQueue.h>
#include <collisionRay.h>
#include <collisionCapsule.h>
#include <plane.h>
#include <stencilAttrib.h>
//#include <transparencyAttrib.h>

// For ImGui
//#include "bridge.h"

// Tools
#include "tools.hpp"
//Grid
#include "grid.hpp"

#define DegToRad(deg) (deg * 3.14159 / 180.0)
#define RadToDeg(rad) (rad * 180.0 / 3.14159)

// Global
extern PandaFramework frwk;
extern WindowFramework *window;
extern PT(MouseWatcher) mWatcher;
extern PT(AsyncTaskManager) taskMgr;

std::string pwd (const NodePath& np);

NodePath getModRoot (const NodePath& obj);

std::pair<LPoint3f, bool> from2d (const LPoint2f& point, const NodePath& cam, const LPoint3f& pos, const LPoint3f& norm);

inline void removeOutline (NodePath& np);
inline void addOutline (NodePath& np);

class Scene { //Controls all objects' movement
	static void eventCollisFunction (const Event* event, void* data);
	void move_cam (LPoint2f mouse); // Move camera and make it look to the center point.
	void collFunc (bool arrows = true); // Check if something is clicked by mouse.
	static AsyncTask::DoneStatus mouseTask (GenericAsyncTask* tsk, void* data); // Repeat while left mouse button is down.
	static AsyncTask::DoneStatus graphicsTask (GenericAsyncTask* task, void* data); // Make tools manager resize (process) arrows. Also moves camera.
	static void mDown (const Event* ev, void* data); // Mouse wheel is down. Increase distance from camera center to camera.
	static void mUp(const Event* ev, void* data); // Mouse wheel is up. Decrease distance from camera center to camera.
	float camRA = 45.0f; // Camera rotation angle.
	float camEA = 45.0f; // Camera elevation angle.
	LPoint3f camCenter = LPoint3f (0.0f, 0.0f, 0.0f);
	std::atomic<float> camRadius = {20.0f}; // Distance between camera and a poitn it looks for.
	static void changeTool (const Event* ev, void* data); // Switch to the next tool.
	static void changeToolMin (const Event* ev, void* data); // Switch to the previous tool.
	static void del (const Event* ev, void* ptr); // Remove selected objects.
	// Mouse position
	LPoint2f prmPos = LPoint2f(0.0,0.0); // Previous mouse position
	LPoint2f prmPos2; // Previous mouse position for graphicsTask
	std::atomic<bool> wasDr = {false};
	std::atomic<bool> DNPick = {false}; // Is true if a GUI element is hovered.
	Tools tMan;
	bool global = true;
	Grid grid;
public:

	// Collisions
	PT(CollisionRay) pickerRay = new CollisionRay(); // A solid for ray casting
	PT(CollisionHandlerQueue) handler = new CollisionHandlerQueue();
	CollisionTraverser traverser = CollisionTraverser("ctraverser");

	// Objects
	NodePath camera;
	NodePath rcroot = NodePath("rcroot"); // Ray casting root
	NodePath mdroot = NodePath("mdroot"); // Models root
	NodePath pcdObj; // Picked object
	std::atomic<bool> isPicked = {false};
	std::atomic<bool> noKeysActs = {false};

	// Picked objects.
	std::unordered_map<void*, NodePath> pcdObjs;

	inline void toMD (NodePath& obj); // Adds a model as a child to mdroot

	enum Orientation {
		Local = 0,
		Global
	};
	void SetOrientation (bool IsGlobal = false); // Global or local tools orientation.
	void SetTool (int tool);
	void setDNPick (bool new_DNPick); // Set DoNotPick variable.
	int GetTool ();

	void pickObj (NodePath& np); // Safely pick an object (save this object and update state of tools).
	bool pickAddObj (NodePath& np); // Safely pick an additional object (if already picked, remove from selected).

	Scene(int argc, char **argv, const std::string &title); // Initialize empty scene on app launch.
};
