#include "scene.hpp"

#define DegToRad(deg) (deg * 3.14159 / 180.0)
#define RadToDeg(rad) (rad * 180.0 / 3.14159)

PandaFramework frwk;
WindowFramework *window;
PT(MouseWatcher) mWatcher;
PT(AsyncTaskManager) taskMgr = AsyncTaskManager::get_global_ptr();

std::string pwd(const NodePath& np) {
	if (np.has_parent()) {
		return pwd(np.get_parent()) + "/" + np.get_name();
	} else {
		return np.get_name();
	}
}

NodePath getModRoot(const NodePath& obj) {
	if (obj.node()->get_type_index() == ModelRoot::get_class_type().get_index()
			|| obj.node()->get_type_index() == CollisionNode::get_class_type().get_index()) {
		return obj;
	} else {
		if (obj.has_parent()) {
			return getModRoot(obj.get_parent());
		} else {
			return NodePath::fail();
		}
	}
}

std::pair<LPoint3f, bool> from2d(const LPoint2f& point, const NodePath& cam, const LPoint3f& pos, const LPoint3f& norm) {
	LPlanef plane(norm, pos);
	LPoint3f np, fp;
	if (DCAST(Camera, cam.node()->get_child(0))->get_lens(0)->extrude(point, np, fp)) {
		np = cam.get_parent().get_relative_point(cam, np);
		fp = cam.get_parent().get_relative_point(cam, fp);
		LPoint3f res;
		if (plane.intersects_line(res, np, fp)) {
			return std::pair<LPoint3f, bool>(res - pos, true);
		} else {
			return std::pair<LPoint3f, bool>(LPoint3f(), false);
		}
	} else {
		return std::pair<LPoint3f, bool>(LPoint3f(), false);
	}
}

inline void removeOutline(NodePath& np) {
	if (!np.node()->is_collision_node()) {
		np.clear_attrib(StencilAttrib::get_class_type());
		for (unsigned long int i = 0; i < np.get_num_children(); i++) {
			if (np.get_child(i).has_tag("internal_editor_outline")) {
				np.get_child(i).remove_node();
			}
		}
	}
}
inline void addOutline(NodePath& np) {
	if (!np.node()->is_collision_node()) {
		NodePath par = np.get_parent();
		NodePathCollection chldrn = np.get_parent().get_children();
		for (unsigned long int i = 0; i < chldrn.get_num_paths(); i++) {
			if (chldrn.get_path(i).has_tag("obj")) {
				chldrn.get_path(i).detach_node();
			} else {
				chldrn.remove_path(chldrn.get_path(i));
			}
		}
		NodePathCollection children = np.get_children();
		for (long int i = children.get_num_paths() - 1; i >= 0; i--) {
			if (children.get_path(i).has_tag("obj")) {
				children.get_path(i).detach_node();
			} else {
				children.remove_path(children.get_path(i));
			}
		}
		np.reparent_to(window->get_render());
		LPoint3f pos = np.get_pos(window->get_render());
		LPoint3f scale = np.get_scale(window->get_render());
		LPoint3f hpr = np.get_hpr(window->get_render());
		np.set_pos(window->get_render(), 0.0f, 0.0f, 0.0f);
		np.set_scale(1.0f, 1.0f, 1.0f);
		np.set_hpr(0.0f, 0.0f, 0.0f);
		NodePath outl = np.attach_new_node("EditorSelectedOutline");
		np.copy_to(outl);
		outl.set_tag("internal_editor_outline", "y");
		outl.set_pos(0.0f, 0.0f, 0.0f);
		outl.set_collide_mask(BitMask32::all_off());
		np.set_attrib(StencilAttrib::make(true, StencilAttrib::M_always, StencilAttrib::SO_keep, StencilAttrib::SO_keep, StencilAttrib::SO_replace, 1, 0xFF, 0xFF));
		outl.set_scale(1.03f);
		outl.set_texture_off(1);
		outl.set_light_off(1);
		outl.set_color(LColorf(1, 0, 0, 1), 1);
		outl.set_depth_test(false);
		outl.set_attrib(StencilAttrib::make(true, StencilAttrib::M_not_equal, StencilAttrib::SO_keep, StencilAttrib::SO_keep, StencilAttrib::SO_replace, 1, 0xFF, 0x00));
		np.set_pos(window->get_render(), pos);
		np.set_scale(window->get_render(), scale);
		np.set_hpr(window->get_render(), hpr);
		chldrn.reparent_to(par);
		children.reparent_to(np);
	}
}

//							SCENE

void Scene::eventCollisFunction(const Event* event, void* data) {
	Scene* scn = static_cast<Scene*>(data);
	if (!scn->DNPick) {
		scn->prmPos = mWatcher->get_mouse();
		scn->collFunc(true);
		taskMgr->add(new GenericAsyncTask("mTsk", &Scene::mouseTask, data)); // Start an async task.
	}
}
void Scene::move_cam(LPoint2f mouse) { // Move camera and make it look to the center point.
	mouse = mouse - prmPos2; // Get delta.
	double dt = ClockObject::get_global_clock()->get_dt();
	if (mWatcher->is_button_down(MouseButton::two())) {
		if (mWatcher->is_button_down(KeyboardButton::shift())) { // It is a hack which lets check if keyboard button is pressed with MouseWatcher.
			std::pair<LPoint3f, bool> pr = from2d(-mouse, camera, camCenter, camera.get_pos()
					- camCenter);
			if (pr.second) {
				pr.first *= (50.0f * dt);
				camCenter += pr.first;
			}
		} else {
			camRA -= mouse.get_x() * 5000.0f * dt;
			camEA += mouse.get_y() * 5000.0f * dt;
			camRA = camRA > 360.0f ? camRA - 360.0f : camRA;
			camRA = camRA < 0 ? camRA + 360.0f : camRA;
			camEA = camEA < 1.0f ? 1.0f : camEA;
			camEA = camEA > 179.0f ? 179.0f : camEA;
		}
	}
	LPoint3f camPos;
	camPos.set_x(camCenter.get_x() + camRadius * sin(DegToRad(camEA)) * cos(DegToRad(camRA)));
	camPos.set_y(camCenter.get_y() + camRadius * sin(DegToRad(camEA)) * sin(DegToRad(camRA)));
	camPos.set_z(camCenter.get_z() + camRadius * cos(DegToRad(camEA)));
	camera.set_pos(camPos);
	camera.look_at(camCenter);
}
void Scene::collFunc(bool arrows) { // Check if something is clicked by mouse.
	if (mWatcher->has_mouse()) {
		if (window->get_graphics_window()) {
			tMan.setToolnum(-1); // We think mouse doesn't point to arrow.
			wasDr = false; // Mouse wasn't dragged since start of click.
			bool shift = mWatcher->is_button_down(KeyboardButton::shift());
			if (!arrows && !shift) {
				isPicked = false; // If we want to pick a new object we must reset previous.
				if (!pcdObj.is_empty()) {
					removeOutline (pcdObj);
				}
				//pcdObjs.clear();
			}
			if (isPicked || !arrows) { // If there is a picked object, there are arrows. We need to check if mouse points toone of them.
				LPoint2f mpos = mWatcher->get_mouse();
				pickerRay->set_from_lens(window->get_camera(0), mpos.get_x(), mpos.get_y());
				if (arrows) {
					traverser.traverse(tMan.toolroot()); // Look for collisions with arrows.
				} else {
					traverser.traverse(mdroot); // Look for collisions with models.
				}
				handler->sort_entries();
				bool was = false;
				for (unsigned int i = 0; i < handler->get_num_entries(); i++) {
					NodePath entry = handler->get_entry(i)->get_into_node_path();
					if (arrows) { // If we want to pick a tool
						tMan.setupTool(entry, pcdObj, rcroot, camera, mWatcher);
					} else if (!arrows && !was) { // Save picked object.
						was = true;
						if (!shift) {
							pcdObj = getModRoot(entry);
							if (pcdObj.get_error_type() != NodePath::ErrorType::ET_fail) {
								addOutline (pcdObj);
								isPicked = true;
								tMan.changeTool(tMan.getTool(), global, true, pcdObj.get_pos(rcroot), pcdObj.get_hpr(rcroot), camera, rcroot);
							}
						} else {
							NodePath entr = getModRoot(entry);
							if (isPicked) {
								if (pcdObj.node() == entr.node()) {
									if (!pcdObj.is_empty()) {
										removeOutline (pcdObj);
									}
									if (pcdObjs.size() > 0) {
										pcdObj = (*pcdObjs.begin()).second;
										pcdObjs.erase(pcdObjs.begin());
										tMan.changeTool(tMan.getTool(), global, true, pcdObj.get_pos(rcroot), pcdObj.get_hpr(rcroot), camera, rcroot);
									} else {
										isPicked = false;
									}

								} else {
									if (pcdObjs.find(entr.node()) != pcdObjs.end()) {
										removeOutline(entr);
										pcdObjs.erase(entr.node());
									} else {
										addOutline(entr);
										pcdObjs.insert(std::pair<void*, NodePath>(entr.node(), entr));
									}
								}
							} else {
								if (!pcdObj.is_empty()) {
									removeOutline (pcdObj);
								}
								addOutline(entr);
								pcdObj = entr;
								isPicked = true;
								tMan.changeTool(tMan.getTool(), global, true, pcdObj.get_pos(rcroot), pcdObj.get_hpr(rcroot), camera, rcroot);
							}

						}
					}
				}
				if (!arrows && !isPicked) {
					tMan.hideTools();
					for (std::pair<void*, NodePath> i : pcdObjs) {
						removeOutline(i.second);
					}
					pcdObjs.clear();
				}
			}
		}
	}
}
AsyncTask::DoneStatus Scene::mouseTask(GenericAsyncTask* tsk, void* data) { // Repeat while left mouse button is down.
	Scene* scn = static_cast<Scene*>(data);
	if (mWatcher->has_mouse() && window->get_graphics_window()) {
		if (!mWatcher->is_button_down(MouseButton::one())) {
			if (scn->isPicked) {
				scn->tMan.postProcessObj(scn->pcdObj, scn->rcroot); // Post-process object and tool if needed.
			}
			if (!scn->wasDr || !scn->isPicked) { // If a mouse wasn't dragged of there is no picked object, pick something.
				scn->collFunc(false);
			}
			scn->tMan.setToolnum(-1);
			return AsyncTask::DS_done;
		}
		if (mWatcher->get_mouse() != scn->prmPos) { // Check if mouse was dragged.
			scn->wasDr = true;
			if (scn->isPicked) {
				scn->tMan.processObj(scn->pcdObj, scn->pcdObjs, scn->rcroot, mWatcher); // Make tools manager process picked object (move, resize or rotate).
			}
			scn->prmPos = mWatcher->get_mouse(); // Save mouse position.
		}
	}
	return AsyncTask::DS_cont; // Continue the task.
}
AsyncTask::DoneStatus Scene::graphicsTask(GenericAsyncTask* task, void* data) { // Make tools manager resize (process) arrows. Also moves camera.
	Scene* scn = static_cast<Scene*>(data);
	scn->tMan.processTool(scn->camera, scn->rcroot);
	LPoint3f pos = scn->camCenter;
	pos.set_z(0.0f);
	scn->grid.place(pos, 5, 0.125f);
	if (window->get_graphics_window() && mWatcher->has_mouse()) {
		scn->move_cam(mWatcher->get_mouse());
		scn->prmPos2 = mWatcher->get_mouse();
	}
	return AsyncTask::DS_cont;
}
void Scene::mUp(const Event* ev, void* data) { // Mouse wheel is up. Decrease distance from camera center to camera.
	Scene* scn = static_cast<Scene*>(data);
	if (!scn->DNPick) {
		ClockObject::get_global_clock()->get_dt();
		scn->camRadius =
				scn->camRadius - 100.0f * ClockObject::get_global_clock()->get_dt() < 1.0f ?
						1.0f : scn->camRadius - 100.0f * ClockObject::get_global_clock()->get_dt();
	}
}
void Scene::mDown(const Event* ev, void* data) { // Mouse wheel is down. Increase distance from camera center to camera.
	Scene* scn = static_cast<Scene*>(data);
	if (!scn->DNPick) {
		ClockObject::get_global_clock()->get_dt();
		scn->camRadius = scn->camRadius + 100.0f * ClockObject::get_global_clock()->get_dt();
	}
}
void Scene::changeTool(const Event* ev, void* data) { // Switch to the next tool.
	Scene* scn = static_cast<Scene*>(data);
	if (!scn->noKeysActs) {
		if (scn->isPicked) {
			if (scn->tMan.getTool() != 2) {
				scn->tMan.changeTool(scn->tMan.getTool() + 1, scn->global, true, scn->pcdObj.get_pos(scn->rcroot), scn->pcdObj.get_hpr(scn->rcroot), scn->camera, scn->rcroot);
			} else {
				scn->tMan.changeTool(0, scn->global, true, scn->pcdObj.get_pos(scn->rcroot), scn->pcdObj.get_hpr(scn->rcroot), scn->camera, scn->rcroot);
			}
		} else {
			if (scn->tMan.getTool() != 2) {
				scn->tMan.changeTool(scn->tMan.getTool() + 1, scn->global, false);
			} else {
				scn->tMan.changeTool(0, scn->global, false);
			}
		}
	}
}
void Scene::changeToolMin(const Event* ev, void* data) { // Switch to the previous tool.
	Scene* scn = static_cast<Scene*>(data);
	if (!scn->noKeysActs) {
		if (scn->isPicked) {
			if (scn->tMan.getTool() != 0) {
				scn->tMan.changeTool(scn->tMan.getTool() - 1, scn->global, true, scn->pcdObj.get_pos(scn->rcroot), scn->pcdObj.get_hpr(scn->rcroot), scn->camera, scn->rcroot);
			} else {
				scn->tMan.changeTool(2, scn->global, true, scn->pcdObj.get_pos(scn->rcroot), scn->pcdObj.get_hpr(scn->rcroot), scn->camera, scn->rcroot);
			}
		} else {
			if (scn->tMan.getTool() != 0) {
				scn->tMan.changeTool(scn->tMan.getTool() - 1, scn->global, false);
			} else {
				scn->tMan.changeTool(2, scn->global, false);
			}
		}
	}
}
void Scene::del(const Event* ev, void* ptr) { // Remove selected objects.
	Scene* scn = static_cast<Scene*>(ptr);
	if (scn->isPicked && !scn->noKeysActs) {
		scn->pcdObj.remove_node();
		scn->isPicked = false;
		for (std::pair<void*, NodePath> i : scn->pcdObjs) {
			i.second.remove_node();
		}
		scn->pcdObjs.clear();
	}
}

inline void Scene::toMD(NodePath& obj) { // Adds a model as a child to mdroot
	obj.reparent_to(mdroot);
}

void Scene::SetOrientation(bool IsGlobal) { // Global or local tools orientation.
	global = IsGlobal;
	if (isPicked) {
		// If an object is picked, change displayed tool.
		tMan.changeTool(tMan.getTool(), global, true, pcdObj.get_pos(rcroot), pcdObj.get_hpr(rcroot), camera, rcroot);
	} else {
		// Else change only variable.
		tMan.changeTool(tMan.getTool(), global, false);
	}
}
void Scene::SetTool(int tool) {
	if (isPicked) {
		tMan.changeTool(tool, global, true, pcdObj.get_pos(rcroot), pcdObj.get_hpr(rcroot), camera, rcroot);
	} else {
		tMan.changeTool(tool, global, false);
	}
}
void Scene::setDNPick(bool new_DNPick) { // Set DoNotPick variable.
	DNPick = new_DNPick;
}
int Scene::GetTool() {
	return tMan.getTool();
}

void Scene::pickObj(NodePath& np) { // Safely pick an object (save this object and update state of tools).
	if (np != mdroot) {
		if (!pcdObj.is_empty()) {
			removeOutline(pcdObj);
		}
		pcdObj = getModRoot(np);
		if (pcdObj.get_error_type() != NodePath::ErrorType::ET_fail) {
			addOutline(np);
			isPicked = true;
			tMan.changeTool(tMan.getTool(), global, true, pcdObj.get_pos(rcroot), pcdObj.get_hpr(rcroot), camera, rcroot);
		}
	}
}
bool Scene::pickAddObj(NodePath& np) { // Safely pick an additional object (if already picked, remove from selected).
	if (np != mdroot) {
		if (isPicked) {
			if (pcdObj.node() == np.node()) {
				if (!pcdObj.is_empty()) {
					removeOutline(pcdObj);
				}
				if (pcdObjs.size() > 0) {
					pcdObj = (*pcdObjs.begin()).second;
					pcdObjs.erase(pcdObjs.begin());
					tMan.changeTool(tMan.getTool(), global, true, pcdObj.get_pos(rcroot), pcdObj.get_hpr(rcroot), camera, rcroot);
				} else {
					isPicked = false;
					tMan.hideTools();
				}
				return false;
			} else {
				if (pcdObjs.find(np.node()) != pcdObjs.end()) {
					removeOutline(np);
					pcdObjs.erase(np.node());
					return false;
				} else {
					addOutline(np);
					pcdObjs.insert(std::pair<void*, NodePath>(np.node(), np));
					return true;
				}
			}
		} else {
			if (!pcdObj.is_empty()) {
				removeOutline(pcdObj);
			}
			addOutline(np);
			pcdObj = np;
			isPicked = true;
			tMan.changeTool(tMan.getTool(), global, true, pcdObj.get_pos(rcroot), pcdObj.get_hpr(rcroot), camera, rcroot);
			return true;
		}
	}
	return true;
}

Scene::Scene(int argc, char **argv, const std::string &title) { // Initialize empty scene on app launch.
	// Open the framework and window
	frwk.open_framework(argc, argv);
	frwk.set_window_title(title);
	window = frwk.open_window();
	window->enable_keyboard();

	rcroot.reparent_to(window->get_render());
	mdroot.reparent_to(rcroot);
	mdroot.set_name("Root");

	//Setup the camera
	camera = window->get_camera_group();
	LPoint3f camPos;
	camPos.set_x (camCenter.get_x() + camRadius * sin(camEA * 3.14f / 180.0f) * cos(camRA * 3.14 / 180.0f));
	camPos.set_y (camCenter.get_y() + camRadius * sin(camEA * 3.14f / 180.0f) * sin(camRA * 3.14 / 180.0f));
	camPos.set_z (camCenter.get_z() + camRadius * cos (camEA * 3.14f /180));
	camera.set_pos(camPos);// Position
	camera.look_at(camCenter);// Rotation

	//Setup the fog
	PT(Fog) fog = new Fog ("SceneFog");
	window->get_camera(0)->get_display_region(0)->set_clear_color(LColorf(0.3f, 0.3f, 0.3f, 0.0f));
	//fog->set_color(0.41f, 0.41f, 0.41f);
	fog->set_color(window->get_camera(0)->get_display_region(0)->get_clear_color());
	fog->set_mode(Fog::Mode::M_exponential_squared);
	fog->set_exp_density(0.007f);
	window->get_render().set_fog(fog);

	// Get PandaNode->MouseWatcherBase->MouseWatcher* from PandaNode* - downcast.
	mWatcher = DCAST(MouseWatcher, window->get_mouse().node());
	PT(CollisionNode) pickerNode = new CollisionNode("mouseRay");

	// We attach pickerNode (CollisionNode) to the camera (NodePath), move it in the scene graph, and we get its Path.
	NodePath pickerNP = camera.attach_new_node(pickerNode);

	// We set the collide mask to be able to collide this object with another objects.
	BitMask32 pickerMask(BitMask32::bit(1)); pickerMask.set_bit_to(0, 1);
	pickerNode->set_from_collide_mask(pickerMask);
	pickerNode->add_solid(pickerRay);// CollisionNode needs to have a solid. It is a ray.

	// Handler will handle collisions only if one of colliding solids is a collider.
	traverser.add_collider(pickerNP, handler);
	//traverser.show_collisions(window->get_render());

	tMan.Init(window, camera);// Initialize tools
	grid.Init(window->get_render(), window, 200);// Initialize grid

	if (window->get_graphics_window() && mWatcher->has_mouse()) {
		prmPos2 = mWatcher->get_mouse();
	}
	taskMgr->add(new GenericAsyncTask("grtask", &Scene::graphicsTask, this));
	//Event handler
	// Run the function if mouse left button is pressed.
	// Equal to framework.define_key("mouse1", "event", &func, nullptr);
	frwk.get_event_handler().add_hook("mouse1", &Scene::eventCollisFunction, this);
	frwk.get_event_handler().add_hook("shift-mouse1", &Scene::eventCollisFunction, this);
	frwk.get_event_handler().add_hook("wheel_up", &Scene::mUp, this);
	frwk.get_event_handler().add_hook("wheel_down", &Scene::mDown, this);
	frwk.get_event_handler().add_hook("arrow_right", &Scene::changeTool, this);
	frwk.get_event_handler().add_hook("arrow_left", &Scene::changeToolMin, this);
	frwk.get_event_handler().add_hook("delete", &Scene::del, this);
}

