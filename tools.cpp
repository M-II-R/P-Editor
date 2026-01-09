#include "tools.hpp"

#define DegToRad(deg) (deg * 3.14159 / 180.0)
#define RadToDeg(rad) (rad * 180.0 / 3.14159)

LVector3f MultVecs (const LVecBase3f& vec1, const LVecBase3f& vec2) {
	LVector3f res = vec1;
	res.set_x(res.get_x() * vec2.get_x());
	res.set_y(res.get_y() * vec2.get_y());
	res.set_z(res.get_z() * vec2.get_z());
	return res;
}

	void Tools::AddShapeToArrow (NodePath& obj) { // Adds collision shape to arrow when it is created.
		const float r = 5.5;
		PT(CollisionCapsule) caps = new CollisionCapsule (obj.get_x(), obj.get_y(),obj.get_z() - r, obj.get_x(), obj.get_y(),obj.get_z() + r, 0.8);
		PT(CollisionNode) capsND = new CollisionNode("ArrowCS");
		capsND->add_solid(caps); capsND->set_into_collide_mask(BitMask32::bit(0));
		NodePath capsNP = obj.attach_new_node(capsND);
	}
	inline void Tools::AddShapeToCircle (NodePath& obj) { // Adds collision shape to circle (toroid) when it is created.
		obj.set_collide_mask(BitMask32::bit(0)); // I couldn't find convenient collision shape, so we will use circle's geometry instead of it.
	}
	void Tools::AddShapeToSizer (NodePath& obj) { // Adds collision shape to sizer when it is created.
		const float r = 5.5;
		PT(CollisionCapsule) caps = new CollisionCapsule (obj.get_x(), obj.get_y(),obj.get_z() - r, obj.get_x(), obj.get_y(),obj.get_z() + r, 0.8);
		PT(CollisionNode) capsND = new CollisionNode("SizerCS");
		capsND->add_solid(caps); capsND->set_into_collide_mask(BitMask32::bit(0));
		NodePath capsNP = obj.attach_new_node(capsND);
	}
	void Tools::AddShapeToSiserS (NodePath& sizer) { // Adds collision shape to spherical sizer when it is created.
		const float r = 1.6f;
		PT(CollisionSphere) sphere = new CollisionSphere (sizer.get_pos(), r);
		PT(CollisionNode) sphereND = new CollisionNode("SizerCS");
		sphereND->add_solid(sphere); sphereND->set_into_collide_mask(BitMask32::bit(0));
		NodePath sphereNP = sizer.attach_new_node(sphereND);
	}
	void Tools::processArrows (const NodePath& camera) { // Set arrows' scale and their rendering order.
		// Change arrowroot's scale.
		arrowroot.set_scale(arrowroot.get_distance(camera)/35);
		std::pair<NodePath, float> arr[] = {
			{
				arrowroot.get_child(0), arrowroot.get_child(0).get_distance(camera) }, {
				arrowroot.get_child(1), arrowroot.get_child(1).get_distance(camera) }, {
				arrowroot.get_child(2), arrowroot.get_child(2).get_distance(camera) } };
		// Sort circles.
		bool isSorted = false;
		while (!isSorted) {
			bool sort = true;
			for (short int i = 0; i < 2; i++) {
				if (arr[i].second > arr[i + 1].second) {
					sort = false;
					std::pair<NodePath, float> pair = arr[i];
					arr[i] = arr[i + 1];
					arr[i + 1] = pair;
				}
			}
			if (sort == true) {
				isSorted = true;
			}
		}
		// Make arrows be displayed correctly.
		for (short int i = 0; i < 3; i++) {
			arr[i].first.set_bin("fixed", 2 - i);
		}
	}
	void Tools::processCircles(const NodePath& camera) { // Set circles' scale and their rendering order.
		// Change circleroot's scale.
		circleroot.set_scale(arrowroot.get_distance(camera) / 15);
		// Sort circles.
		std::pair<NodePath, float> arr[] = {
			{
				circleroot.get_child(0), (camera.get_pos() - window->get_render().get_relative_point(circleroot.get_child(0), LPoint3f(0.5f, 0.0f,0.0f))).length() }, {
				circleroot.get_child(1), (camera.get_pos() - window->get_render().get_relative_point(circleroot.get_child(1), LPoint3f(0.5f, 0.0f,0.0f))).length() }, {
				circleroot.get_child(2), (camera.get_pos() - window->get_render().get_relative_point(circleroot.get_child(2), LPoint3f(0.5f, 0.0f,0.0f))).length() } };
		bool isSorted = false;
		while (!isSorted) {
			bool sort = true;
			for (short int i = 0; i < 2; i++) {
				if (arr[i].second > arr[i + 1].second) {
					sort = false;
					std::pair<NodePath, float> pair = arr[i];
					arr[i] = arr[i + 1];
					arr[i + 1] = pair;
				}
			}
			if (sort == true) {
				isSorted = true;
			}
		}
		// Make circles be displayed correctly and rotate them.
		for (short int i = 0; i < 3; i++) {
			arr[i].first.set_bin("fixed", 2 - i);
			LPoint3f point = arr[i].first.get_relative_point(camera, LPoint3f(0,0,0));
			float angle = RadToDeg(atan2(point.get_y(),point.get_x()));
			arr[i].first.get_child(0).set_hpr(angle, 0, 0);
		}
	}
	void Tools::processSizers (const NodePath& camera) { // Set sizers' scale and their rendering order.
		// Change arrowroot's scale.
		sizerroot.set_scale(arrowroot.get_distance(camera)/35);
		std::pair<NodePath, float> arr[] = {
			{
				sizerroot.get_child(0), sizerroot.get_child(0).get_distance(camera) }, {
				sizerroot.get_child(1), sizerroot.get_child(1).get_distance(camera) }, {
				sizerroot.get_child(2), sizerroot.get_child(2).get_distance(camera) } };
		// Sort circles.
		bool isSorted = false;
		while (!isSorted) {
			bool sort = true;
			for (short int i = 0; i < 2; i++) {
				if (arr[i].second > arr[i + 1].second) {
					sort = false;
					std::pair<NodePath, float> pair = arr[i];
					arr[i] = arr[i + 1];
					arr[i + 1] = pair;
				}
			}
			if (sort == true) {
				isSorted = true;
			}
		}
		// Make sizers be displayed correctly.
		for (short int i = 0; i < 3; i++) {
			arr[i].first.set_bin("fixed", 3 - i);
		}
		sizerroot.get_child(4).look_at(camera);
	}

	void Tools::dragObj (NodePath& pcdObj, const NodePath& rcroot, const MouseWatcher* mWatcher, bool first) { // Drag object with arrows.
		LPoint2f mpos = mWatcher->get_mouse();
		toolRay->set_from_lens(window->get_camera(0), mpos.get_x(), mpos.get_y());
		toolTrav.traverse(arrowroot);
		toolHandl->sort_entries();
		bool r = false;
		for (int i = 0; i < toolHandl->get_num_entries() && !r; i++) {
			if (toolHandl->get_entry(i)->get_into_node_path().get_name() == "plnode") {
				LPoint3f pos = toolHandl->get_entry(i)->get_surface_point(rcroot);
				r = true;
				if (!first) {
					//	pos -= pcdObj.get_pos(rcroot); // With this line program works worse.
					if (glob) {
						LPoint3f delta = pos - prevPos;
						if (toolnum == 0) {
							pcdObj.set_x(rcroot, pcdObj.get_pos(rcroot).get_x() + delta.get_x());
						}
						else if (toolnum == 1) {
							pcdObj.set_y(rcroot, pcdObj.get_pos(rcroot).get_y() + delta.get_y());
						}
						else if (toolnum == 2) {
							pcdObj.set_z(rcroot, pcdObj.get_pos(rcroot).get_z() + delta.get_z());
						}
						arrowroot.set_pos(pcdObj.get_pos(rcroot));
					}
					else {
						LVector3f delta = arrowroot.get_relative_point(rcroot, pos) - arrowroot.get_relative_point(rcroot, prevPos); // Get position relatively to arrowroot, not rcroot.
						//delta = arrowroot.get_relative_point(rcroot, delta);
						if (toolnum == 0) {
							pcdObj.set_x(arrowroot, delta.get_x()); // Now arrowroot's position = (0; 0; 0) for pcdObj
						}
						else if (toolnum == 1) {
							pcdObj.set_y(arrowroot, delta.get_y());
						}
						else if (toolnum == 2) {
							pcdObj.set_z(arrowroot, delta.get_z());
						}
						delta = rcroot.get_relative_vector(arrowroot, delta);
						arrowroot.set_pos(pcdObj.get_pos(rcroot));
					}

				}
				else {
					switch (toolnum) {
					case 0:
						arrowroot.get_child(1).hide();
						arrowroot.get_child(2).hide();
						break;
					case 1:
						arrowroot.get_child(0).hide();
						arrowroot.get_child(2).hide();
						break;
					case 2:
						arrowroot.get_child(0).hide();
						arrowroot.get_child(1).hide();
						break;
					}
				}
				prevPos = pos;
			}
		}
	}
	inline void Tools::postProcArrows () { // Post-processing arrows.
		for (short int i = 0; i < arrowroot.get_num_children(); i++) {
				arrowroot.get_child(i).show();
		}
	}
	void Tools::rotObj(NodePath& pcdObj, const NodePath& rcroot, const MouseWatcher* mWatcher, bool first) { // Rotate object with circles.
		LPoint2f mpos = mWatcher->get_mouse();
		toolRay->set_from_lens(window->get_camera(0), mpos.get_x(), mpos.get_y());
		toolTrav.traverse(circleroot);
		toolHandl->sort_entries();
		bool r = false;
		for (int i = 0; i < toolHandl->get_num_entries() && !r; i++) {
			if (toolHandl->get_entry(i)->get_into_node_path().get_name() == "plnode") {
				r = true;
				if (glob) {
				LPoint3f pos = toolHandl->get_entry(i)->get_surface_point(rcroot);
				pos -= pcdObj.get_pos(rcroot);
					if (!first) {
						if (toolnum == 0) {
							float ang = LVector2f(pos.get_yz()).normalized().signed_angle_deg(LVector2f(prevPos.get_yz()).normalized());
							LQuaternionf quat;
							quat.set_from_axis_angle(-ang, LVector3f(1, 0, 0));
							pcdObj.set_quat(rcroot, pcdObj.get_quat(rcroot) * quat);
						} else if (toolnum == 1) {
							float ang = LVector2f(pos.get_xz()).normalized().signed_angle_deg(LVector2f(prevPos.get_xz()).normalized());
							LQuaternionf quat;
							quat.set_from_axis_angle(ang, LVector3f(0, 1, 0));
							pcdObj.set_quat(rcroot, pcdObj.get_quat(rcroot) * quat);
						} else if (toolnum == 2) {
							float ang = LVector2f(pos.get_xy()).normalized().signed_angle_deg(LVector2f(prevPos.get_xy()).normalized());
							LQuaternionf quat;
							quat.set_from_axis_angle(-ang, LVector3f(0, 0, 1));
							pcdObj.set_quat(rcroot, pcdObj.get_quat(rcroot) * quat);
						}
					}
					prevPos = pos;
				}
				else {
					LPoint3f pos = toolHandl->get_entry(i)->get_surface_point(circleroot);
					if (!first) {
						if (toolnum == 0) {
							float ang = LVector2f(pos.get_yz()).normalized().signed_angle_deg(LVector2f(prevPos.get_yz()).normalized());
							LQuaternionf quat;
							quat.set_from_axis_angle(-ang, LVector3f(1, 0, 0));
							pcdObj.set_quat(circleroot, pcdObj.get_quat(circleroot) * quat);
						} else if (toolnum == 1) {
							float ang = LVector2f(pos.get_xz()).normalized().signed_angle_deg(LVector2f(prevPos.get_xz()).normalized());
							LQuaternionf quat;
							quat.set_from_axis_angle(ang, LVector3f(0, 1, 0));
							pcdObj.set_quat(circleroot, pcdObj.get_quat(circleroot) * quat);
						} else if (toolnum == 2) {
							float ang = LVector2f(pos.get_xy()).normalized().signed_angle_deg(LVector2f(prevPos.get_xy()).normalized());
							LQuaternionf quat;
							quat.set_from_axis_angle(-ang, LVector3f(0, 0, 1));
							pcdObj.set_quat(circleroot, pcdObj.get_quat(circleroot) * quat);
						}
					}
					prevPos = pos;
				}
				if (first) { // Hide some circles
					switch (toolnum) {
					case 0:
						circleroot.get_child(1).hide();
						circleroot.get_child(2).hide();
						break;
					case 1:
						circleroot.get_child(0).hide();
						circleroot.get_child(2).hide();
						break;
					case 2:
						circleroot.get_child(0).hide();
						circleroot.get_child(1).hide();
						break;
					}
				}
			}
		}
	}
	inline void Tools::postProcCircles (const NodePath& pcdObj, const NodePath& rcroot) { // Rotate circles after local transformation.
		for (short int i = 0; i < circleroot.get_num_children(); i++) {
				circleroot.get_child(i).show();
		}
		if (!glob) {
			circleroot.set_hpr(rcroot, pcdObj.get_hpr(rcroot));
		}
	}
	void Tools::resizeObj (NodePath& pcdObj, const NodePath& rcroot, const MouseWatcher* mWatcher, bool first) { // Resize object with sizers.
		LPoint2f mpos = mWatcher->get_mouse();
		toolRay->set_from_lens(window->get_camera(0), mpos.get_x(), mpos.get_y());
		toolTrav.traverse(sizerroot);
		toolHandl->sort_entries();
		bool r = false;
		for (int i = 0; i < toolHandl->get_num_entries() && !r; i++) {
			if (toolHandl->get_entry(i)->get_into_node_path().get_name() == "plnode") {
				r = true;
				if (glob) {
					LPoint3f pos = toolHandl->get_entry(i)->get_surface_point(rcroot);
					pos -= pcdObj.get_pos(rcroot);
					if (!first) {
						LVector3f delta = pos - prevPos;
						if (toolnum == 0) {
							LVector3f vec = pcdObj.get_relative_vector(rcroot, LVector3f(pcdObj.get_x(rcroot) + delta.get_x(), pcdObj.get_y(rcroot), pcdObj.get_z(rcroot)));
							if (delta.get_x() > 0) {
								vec.set_x(abs(vec.get_x()));
								vec.set_y(abs(vec.get_y()));
								vec.set_z(abs(vec.get_z()));
							} else {
								vec.set_x(-abs(vec.get_x()));
								vec.set_y(-abs(vec.get_y()));
								vec.set_z(-abs(vec.get_z()));
							}
							pcdObj.set_scale(rcroot, pcdObj.get_scale(rcroot) + MultVecs(pcdObj.get_scale(rcroot), vec));
						} else if (toolnum == 1) {
							LVector3f vec = pcdObj.get_relative_vector(rcroot, LVector3f(pcdObj.get_x(rcroot), pcdObj.get_y(rcroot) + delta.get_y(), pcdObj.get_z(rcroot)));
							if (delta.get_y() > 0) {
								vec.set_x(abs(vec.get_x()));
								vec.set_y(abs(vec.get_y()));
								vec.set_z(abs(vec.get_z()));
							} else {
								vec.set_x(-abs(vec.get_x()));
								vec.set_y(-abs(vec.get_y()));
								vec.set_z(-abs(vec.get_z()));
							}
							pcdObj.set_scale(rcroot, pcdObj.get_scale(rcroot) + MultVecs(pcdObj.get_scale(rcroot), vec));
						} else if (toolnum == 2) {
							LVector3f vec = pcdObj.get_relative_vector(rcroot, LVector3f(pcdObj.get_x(rcroot), pcdObj.get_y(rcroot), pcdObj.get_z(rcroot) + delta.get_z()));
							if (delta.get_z() > 0) {
								vec.set_x(abs(vec.get_x()));
								vec.set_y(abs(vec.get_y()));
								vec.set_z(abs(vec.get_z()));
							} else {
								vec.set_x(-abs(vec.get_x()));
								vec.set_y(-abs(vec.get_y()));
								vec.set_z(-abs(vec.get_z()));
							}
							pcdObj.set_scale(rcroot, pcdObj.get_scale(rcroot) + MultVecs(pcdObj.get_scale(rcroot), vec));
						}
						else if (toolnum == 3) {
							pcdObj.set_scale(rcroot, pcdObj.get_scale(rcroot) + (pos.length() > prevPos.length() ? pcdObj.get_scale(rcroot) * (pos - prevPos).length() : -pcdObj.get_scale(rcroot) * (pos - prevPos).length()));
						}
					}
					prevPos = pos;
				}
				else { // Local axes
					LPoint3f pos = toolHandl->get_entry(i)->get_surface_point(sizerroot);
					if (!first) {
						LVector3f delta = pos - prevPos;
						if (toolnum == 0) {
							pcdObj.set_scale(rcroot, pcdObj.get_scale(rcroot).get_x() + delta.get_x(), pcdObj.get_scale(rcroot).get_y(),  pcdObj.get_scale(rcroot).get_z());
						}
						else if (toolnum == 1){
							pcdObj.set_scale(rcroot, pcdObj.get_scale(rcroot).get_x(), pcdObj.get_scale(rcroot).get_y() + delta.get_y(),  pcdObj.get_scale(rcroot).get_z());
						}
						else if (toolnum == 2) {
							pcdObj.set_scale(rcroot, pcdObj.get_scale(rcroot).get_x(), pcdObj.get_scale(rcroot).get_y(),  pcdObj.get_scale(rcroot).get_z() + delta.get_z());
						}
						else if (toolnum == 3) {
							pcdObj.set_scale(rcroot, pcdObj.get_scale(rcroot) + (pos.length() > prevPos.length() ? pcdObj.get_scale(rcroot) * (pos - prevPos).length() : -pcdObj.get_scale(rcroot) * (pos - prevPos).length()));
						}
					}
					prevPos = pos;
				}
				if (first) {
					switch (toolnum) {
					case 0:
						sizerroot.get_child(1).hide();
						sizerroot.get_child(2).hide();
						sizerroot.get_child(3).hide();
						sizerroot.get_child(4).hide();
						break;
					case 1:
						sizerroot.get_child(0).hide();
						sizerroot.get_child(2).hide();
						sizerroot.get_child(3).hide();
						sizerroot.get_child(4).hide();
						break;
					case 2:
						sizerroot.get_child(0).hide();
						sizerroot.get_child(1).hide();
						sizerroot.get_child(3).hide();
						sizerroot.get_child(4).hide();
						break;
					case 3:
						sizerroot.get_child(0).hide();
						sizerroot.get_child(1).hide();
						sizerroot.get_child(2).hide();
						break;
					}
				}
			}
		}
	}
	inline void Tools::postProcSizers () { // Rotate circles after local transformation.
		for (short int i = 0; i < sizerroot.get_num_children(); i++) {
				sizerroot.get_child(i).show();
		}
	}
	inline void Tools::SetPlaneRot (const NodePath& arrow, const NodePath& camera) { // Rotate a plane that is placed to arrow position to make it look to the camera.
		LPoint3f point = arrow.get_relative_point(camera, LPoint3f(0,0,0));
		float angle = RadToDeg(atan2(point.get_y(),point.get_x()));
		plane.set_hpr(angle, 0, 0);
	}
	inline void Tools::crlSetPlaneRot (const NodePath& camera) { // Same thing as SetPlaneRot but for circles.
		plane.set_hpr(0.0f, 0.0f, 90.0f);
		if (camera.get_pos(circleroot.get_child(toolnum)).get_z() > 0) {
			plane.set_r(-90.0f);
		}
	}
	Tools::Tools () { window = nullptr; }
	void Tools::Init (WindowFramework* win, NodePath& camera) { // Initialize tools.
		window = win;
		PT(CollisionNode) tRayNode = new CollisionNode ("toolRay");
		NodePath tRayPath = camera.attach_new_node(tRayNode);
		tRayNode->set_from_collide_mask(BitMask32::bit(2));
		tRayNode->add_solid(toolRay);
		toolTrav.add_collider(tRayPath, toolHandl);
		//toolTrav.show_collisions(camera.get_parent().has_parent() ? camera.get_parent().get_parent() : camera.get_parent()); // For debugging.

		PT(CollisionPlane) cpln = new CollisionPlane (LPlanef(LVector3f(1,0,0), LPoint3f(0,0,0)));
		plnd->add_solid(cpln);
		plnd->set_into_collide_mask(BitMask32::bit(2)); // Unique collide mask. Needs an another ray.
		plane = window->get_render().attach_new_node(plnd);
		plane.detach_node();

		// Arrows
		NodePath arrR1 = NodePath ("arrR1"); arrR1.reparent_to(arrowroot);
		arrR1.set_bin("fixed", 0); arrR1.set_depth_test(false); arrR1.set_depth_write(false);
		NodePath arrR = window->load_model(arrR1, "assets/AR.bam");
		arrR1.set_scale(0.2); AddShapeToArrow(arrR); arrR1.set_hpr(arrR1.get_hpr().get_x(), arrR1.get_hpr().get_y(), arrR1.get_hpr().get_z() + 90);
		arrR1.set_pos(2.5f, 0, 0);

		NodePath arrG1 = NodePath ("arrG1"); arrG1.reparent_to(arrowroot);
		arrG1.set_bin("fixed", 0); arrG1.set_depth_test(false); arrG1.set_depth_write(false);
		NodePath arrG = window->load_model(arrG1, "assets/AG.bam");
		arrG1.set_scale(0.2); AddShapeToArrow(arrG); arrG1.set_hpr(arrG1.get_hpr().get_x(), arrG1.get_hpr().get_y() - 90,arrG1.get_hpr().get_z());
		arrG1.set_pos(0, 2.5f, 0);

		NodePath arrB1 = NodePath ("arrB1"); arrB1.reparent_to(arrowroot);
		arrB1.set_bin("fixed", 0); arrB1.set_depth_test(false); arrB1.set_depth_write(false);
		NodePath arrB = window->load_model(arrB1, "assets/AB.bam");
		arrB1.set_scale(0.2); AddShapeToArrow(arrB);
		arrB1.set_pos(0, 0, 2.5f);

		// Circles
		NodePath crlR1 ("crlR1"); crlR1.reparent_to (circleroot); crlR1.set_bin("fixed", 0); crlR1.set_depth_test(false); crlR1.set_depth_write(false);
		NodePath crlR = window->load_model(crlR1, "assets/CircleR1.bam"); AddShapeToCircle(crlR); crlR.set_name("crlR"); crlR1.set_hpr(0, 90, 90);

		NodePath crlG1 ("crlG1"); crlG1.reparent_to (circleroot); crlG1.set_bin("fixed", 0); crlG1.set_depth_test(false); crlG1.set_depth_write(false);
		NodePath crlG = window->load_model(crlG1, "assets/CircleG1.bam"); AddShapeToCircle(crlG); crlG.set_name("crlG"); crlG1.set_hpr(0, 90, 0);

		NodePath crlB1 ("crlB1"); crlB1.reparent_to (circleroot); crlB1.set_bin("fixed", 0); crlB1.set_depth_test(false); crlB1.set_depth_write(false);
		NodePath crlB = window->load_model(crlB1, "assets/CircleB1.bam"); AddShapeToCircle(crlB); crlB.set_name("crlB"); crlB1.set_hpr(45, 0, 0);

		//Sizers
		NodePath szrR1 = NodePath ("szrR1"); szrR1.reparent_to(sizerroot);
		szrR1.set_bin("fixed", 0); szrR1.set_depth_test(false); szrR1.set_depth_write(false);
		NodePath szrR = window->load_model(szrR1, "assets/SizerR.bam");
		szrR1.set_scale(0.2); AddShapeToSizer(szrR); szrR1.set_hpr(szrR1.get_hpr().get_x(), szrR1.get_hpr().get_y(), szrR1.get_hpr().get_z() + 90);
		szrR1.set_pos(2.5f, 0, 0);

		NodePath szrG1 = NodePath ("szrG1"); szrG1.reparent_to(sizerroot);
		szrG1.set_bin("fixed", 0); szrG1.set_depth_test(false); szrG1.set_depth_write(false);
		NodePath szrG = window->load_model(szrG1, "assets/SizerG.bam");
		szrG1.set_scale(0.2); AddShapeToSizer(szrG); szrG1.set_hpr(szrG1.get_hpr().get_x(), szrG1.get_hpr().get_y() - 90,szrG1.get_hpr().get_z());
		szrG1.set_pos(0, 2.5f, 0);

		NodePath szrB1 = NodePath ("szrB1"); szrB1.reparent_to(sizerroot);
		szrB1.set_bin("fixed", 0); szrB1.set_depth_test(false); szrB1.set_depth_write(false);
		NodePath szrB = window->load_model(szrB1, "assets/SizerB.bam");
		szrB1.set_scale(0.2); AddShapeToSizer(szrB);
		szrB1.set_pos(0, 0, 2.5f);

		NodePath szrS = window->load_model(sizerroot, "assets/SizerS.bam"); szrS.set_name ("szrS"); szrS.set_transparency(TransparencyAttrib::Mode::M_alpha); szrS.set_alpha_scale(0.2f);
		szrS.set_bin("fixed", 0); szrS.set_depth_test(false); szrS.set_depth_write(false); szrS.set_scale(0.7f); AddShapeToSiserS(szrS);

		NodePath szrC = window->load_model(sizerroot, "assets/SizerC.bam"); szrC.set_name ("szrC");
		szrC.set_bin("fixed", 4); szrC.set_depth_test(false); szrC.set_depth_write(false); AddShapeToCircle(szrC); szrC.set_scale(4.0f);
	}
	void Tools::processTool (NodePath& camera, NodePath& rcroot) { // Make current tool look COOL! Process them every frame.
		switch (tool) {
		case 0:
			if (arrowroot.get_parent() == rcroot) {
				processArrows(camera);
			}
			break;
		case 1:
			if (circleroot.get_parent() == rcroot) {
				processCircles(camera);
			}
			break;
		case 2:
			if (sizerroot.get_parent() == rcroot) {
				processSizers(camera);
			}
			break;
		}
	}
	void Tools::processObj(NodePath& pcdObj, std::unordered_map<void*, NodePath>& pcdObjs, const NodePath& rcroot, const MouseWatcher* mWatcher, bool first) { // Operate(drag, resize or rotate) object with current tool.
		std::unordered_map<void*, NodePath> parents;
		for (auto i : pcdObjs) {
			i.second.set_mat(i.second.get_mat(pcdObj));
			parents.insert(std::pair<void*, NodePath>(i.second.node(), i.second.get_parent()));
			i.second.reparent_to(pcdObj);
		}
		switch (tool) {
		case 0:
			dragObj(pcdObj, rcroot, mWatcher, first);
			break;
		case 1:
			rotObj(pcdObj, rcroot, mWatcher, first);
			break;
		case 2:
			resizeObj(pcdObj, rcroot, mWatcher, first);
			break;
		}
		for (auto i : pcdObjs) {
			//i.second.set_mat(i.second.get_mat(rcroot));
			i.second.set_mat(i.second.get_mat(parents.at(i.second.node())));
			i.second.reparent_to(parents.at(i.second.node()));
		}
	}
	void Tools::postProcessObj (const NodePath& pcdObj, const NodePath& rcroot) {
		switch (tool) {
		case 0:
			postProcArrows();
			break;
		case 1:
			postProcCircles (pcdObj, rcroot); // If orientation isn't global, we need to rotate circles to make them have the same angle as pcdObj.
			break;
		case 2:
			postProcSizers();
			break;
		}
	}
	void Tools::changeTool(short int num, bool global, bool reShow, const LPoint3f& pos, const LVector3f& rot, const NodePath& camera, const NodePath& rcroot) { // Change current tool to another.
		if (reShow) {
			hideTools();
			switch (num) {
			case 0:
				arrowroot.reparent_to(rcroot);
				arrowroot.set_pos(pos);
				if (!global) {
					arrowroot.set_hpr(rot);
				}
				else {
					arrowroot.set_hpr(0.0f, 0.0f, 0.0f);
				}
				processArrows(camera);
				break;
			case 1:
				circleroot.reparent_to(rcroot);
				circleroot.set_pos(pos);
				if (!global) {
					circleroot.set_hpr(rot);
				}
				else {
					circleroot.set_hpr(0.0f, 0.0f, 0.0f);
				}
				processCircles(camera);
				break;
			case 2:
				sizerroot.reparent_to(rcroot);
				sizerroot.set_pos(pos);
				if (!global) {
					sizerroot.set_hpr(rot);
				}
				else {
					sizerroot.set_hpr(0.0f, 0.0f, 0.0f);
				}
				processSizers(camera);
				break;
			}
		}
		glob = global;
		tool = num;
		toolnum = -1;
	}
	void Tools::setToolnum (short int num) {
		toolnum = num;
	}
	void Tools::setupTool (NodePath& entry, NodePath& pcdObj, const NodePath& rcroot, const NodePath& camera, const MouseWatcher* mWatcher) { // Prepare current tool for operations.
		if (entry.get_name() == "ArrowCS") { // If it is a collision shape of arrow.
			if (entry.get_parent().get_parent().get_name() == "arrR1") {
				toolnum = 0;
			} else if (entry.get_parent().get_parent().get_name() == "arrG1") {
				toolnum = 1;
			} else if (entry.get_parent().get_parent().get_name() == "arrB1") {
				toolnum = 2;
			}
			if (toolnum != -1) {
				plane.reparent_to(arrowroot.get_child(toolnum)); // Move plane to arrow.
				SetPlaneRot(arrowroot.get_child(toolnum), camera);
				dragObj(pcdObj, rcroot, mWatcher, true); // Set prevPos variable to current mouse pos. We need pcdObj to avoid temporary nodes (I don't like them).
			}
		} else if (entry.get_name() == "Torus") {
			if (entry.get_parent().get_parent().get_name() == "crlR") {
				toolnum = 0;
			} else if (entry.get_parent().get_parent().get_name() == "crlG") {
				toolnum = 1;
			} else if (entry.get_parent().get_parent().get_name() == "crlB") {
				toolnum = 2;
			}
			if (toolnum != -1) {
				plane.reparent_to(circleroot.get_child(toolnum));
				crlSetPlaneRot(camera);
				rotObj(pcdObj, rcroot, mWatcher, true); // Set prevPos to current pos.
			}
		} else if (entry.get_name() == "SizerCS") {
			if (entry.get_parent().get_parent().get_name() == "szrR1") {
				toolnum = 0;
			} else if (entry.get_parent().get_parent().get_name() == "szrG1") {
				toolnum = 1;
			} else if (entry.get_parent().get_parent().get_name() == "szrB1") {
				toolnum = 2;
			}
			else if (entry.get_parent().get_name() == "szrS") {
				toolnum = 3;
			}
			if (toolnum != -1) {
				plane.reparent_to(sizerroot.get_child(toolnum));
				if (toolnum != 3) {
					SetPlaneRot(sizerroot.get_child(toolnum), camera);
				}
				else {
					plane.look_at(camera); // We need to set plane rotation for szrS
					plane.set_h(plane, 90);
				}
				resizeObj(pcdObj, rcroot, mWatcher, true);
			}
		}
		else if (entry.get_name() == "SizerC") {
			toolnum = 3;
			plane.reparent_to(sizerroot.get_child(toolnum));
			plane.look_at(camera); // We need to set plane rotation for szrC
			plane.set_h(plane, 90);
			resizeObj(pcdObj, rcroot, mWatcher, true); // SizerC in resizeObj == SizerS.
		}
	}
	void Tools::hideTools () {
		arrowroot.detach_node();
		circleroot.detach_node();
		sizerroot.detach_node();
	}
	short int Tools::getTool () {
		return tool;
	}
	short int Tools::getToolnum() { // Get number of current tool.
		return toolnum;
	}
NodePath& Tools::toolroot () { // Get root of tools - their parent node.
		switch (tool) {
		case 0:
			return arrowroot;
		case 1:
			return circleroot;
		case 2:
			return sizerroot;
		default:
			return arrowroot;
		}
	}
