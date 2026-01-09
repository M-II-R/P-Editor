#include <texturePool.h>
#include <buttonThrower.h>
#include <pgTop.h>
#include <functional>
#include <collisionPolygon.h>
#include <collisionInvSphere.h>
#include <imgui.h>
#include "panda3d_imgui.hpp"
#include <unordered_map>
#include <unordered_set>
#include "nfd.h" // Native file dialog
#include "fast_float.h" // Working std::stof replacement
#include <modelRoot.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <sstream>
#include <cctype>
#include <limits.h>
#include <load_prc_file.h>

#include "scene.hpp"
//#include <clip.h> // For clipboard access
#define PANDA_ROOT scene->mdroot
#define PANDA_WINDOW window
//#include "header.h"

// When compile for Windows:
// #define WINDOWS 1

#define TABS if (tabs != 0) { stream.seekg(tabs, std::ios::cur); }

std::pair<bool, std::string> ExportCppMacro(const Filename& path); // Export scene to C++ macro.
std::pair<bool, std::string> ExportPy(const Filename& path); // Export scene to Python code.

void saveNP(std::ofstream& stream, const NodePath& np, const std::string& tabs=""); // Save NodePath (to editor file).
void loadNP(std::ifstream& stream, const NodePath& parent); // Load NodePath from editor file.

void parseLine (std::vector<std::string>& vec, const std::string& line) { // Removes tabs and spaces from string when loading scene.
	std::string cl;
	for (unsigned long long i = 0; i < line.size(); i++) {
		std::string symb = line.substr(i, 1);
		if(symb != u8"	") {
			if (symb == u8" ") {
				if(cl.size() > 0) {
					vec.push_back(cl);
				}
				cl = "";
			}
			else {
				cl += symb;
			}
		}
	}
	if (cl.size() > 0) {
		vec.push_back(cl);
	}
}

bool checkNP(std::ifstream& stream) { // Check if there is a nodepath in scene file.
	std::ifstream::pos_type pos = stream.tellg();
	std::string line = "";
	if (std::getline(stream, line)) {
		std::vector<std::string> vec;
		parseLine(vec, line);
		if (vec[0] == u8"n" || vec[0] == u8"m" || vec[0] == u8"s") {
			stream.seekg(pos);
			return true;
		}
	}
	stream.seekg(pos);
	return false;
}
std::string floatWithDot(float val) { // Represent float as string with dot and add postfix "f" to it.
	std::stringstream stream;
	stream.imbue(std::locale("C"));
	stream << std::fixed << val;
	return stream.str() + u8"f";
}
std::string floatWithDot2(float val) { // Represent float as string with dot without "f" postfix.
	std::stringstream stream;
	stream.imbue(std::locale("C"));
	stream << std::fixed << val;
	return stream.str();
}

inline void fromPoint(const LPoint3f& pt, float& x, float& y, float& z) { // Write point data to 3 variables.
	x = pt.get_x(); y = pt.get_y(); z = pt.get_z();
}

std::string fromPoint(const LPoint3f& point) { // Convert point to string with 3 numbers having "f" postfix and comma-separated.
	std::string res;
	res += floatWithDot(point.get_x()) + u8", ";
	res += floatWithDot(point.get_y()) + u8", ";
	res += floatWithDot(point.get_z());
	return res;
}
std::string fromPoint2 (const LPoint3f& point) { // Convert point to string with numbers without "f" postfix and without comma.
	std::string res;
	res += floatWithDot2(point.get_x()) + u8", ";
	res += floatWithDot2(point.get_y()) + u8", ";
	res += floatWithDot2(point.get_z());
	return res;
}
std::string fromPoint3 (const LPoint3f& point) { // Convert point to string with comma-separated numbers without "f" postfix.
	std::string res;
	res += floatWithDot2(point.get_x()) + u8" ";
	res += floatWithDot2(point.get_y()) + u8" ";
	res += floatWithDot2(point.get_z());
	return res;
}

void setup_render(Panda3DImGui* panda3d_imgui_helper)
{ // From ImGui helper usage example. Setup render to be ready to work with ImGui.
    auto task_mgr = AsyncTaskManager::get_global_ptr();

    // NOTE: ig_loop has process_events and 50 sort.
    PT(GenericAsyncTask) new_frame_imgui_task = new GenericAsyncTask("new_frame_imgui", [](GenericAsyncTask*, void* user_data) {
        static_cast<Panda3DImGui*>(user_data)->new_frame_imgui();
        return AsyncTask::DS_cont;
    }, panda3d_imgui_helper);
    new_frame_imgui_task->set_sort(0);
    task_mgr->add(new_frame_imgui_task);

    PT(GenericAsyncTask) render_imgui_task = new GenericAsyncTask("render_imgui", [](GenericAsyncTask*, void* user_data) {
        static_cast<Panda3DImGui*>(user_data)->render_imgui();
        return AsyncTask::DS_cont;
    }, panda3d_imgui_helper);
    render_imgui_task->set_sort(40);
    task_mgr->add(render_imgui_task);
}

void setup_button(WindowFramework* window_framework, Panda3DImGui* panda3d_imgui_helper)
{ // From ImGui helper usage example. Bind some events for ImGui.
    if (auto bt = window_framework->get_mouse().find("kb-events"))
    {
        auto ev_handler = EventHandler::get_global_event_handler();

        ButtonThrower* bt_node = DCAST(ButtonThrower, bt.node());
        std::string ev_name;

        ev_name = bt_node->get_button_down_event();
        if (ev_name.empty())
        {
            ev_name = "imgui-button-down";
            bt_node->set_button_down_event(ev_name);
        }
        ev_handler->add_hook(ev_name, [](const Event* ev, void* user_data) {
            const auto& key_name = ev->get_parameter(0).get_string_value();
            const auto& button = ButtonRegistry::ptr()->get_button(key_name);
            static_cast<Panda3DImGui*>(user_data)->on_button_down_or_up(button, true);
        }, panda3d_imgui_helper);
        ev_handler->add_hook("shift-mouse1", [](const Event* ev, void* user_data) {
            //const auto& key_name = ev->get_parameter(0).get_string_value();
            const auto& button = ButtonRegistry::ptr()->get_button("mouse1");
            static_cast<Panda3DImGui*>(user_data)->on_button_down_or_up(button, true);
        }, panda3d_imgui_helper);

        ev_name = bt_node->get_button_up_event();
        if (ev_name.empty())
        {
            ev_name = "imgui-button-up";
            bt_node->set_button_up_event(ev_name);
        }
        ev_handler->add_hook(ev_name, [](const Event* ev, void* user_data) {
            const auto& key_name = ev->get_parameter(0).get_string_value();
            const auto& button = ButtonRegistry::ptr()->get_button(key_name);
            static_cast<Panda3DImGui*>(user_data)->on_button_down_or_up(button, false);
        }, panda3d_imgui_helper);

        ev_name = bt_node->get_keystroke_event();
        if (ev_name.empty())
        {
            ev_name = "imgui-keystroke";
            bt_node->set_keystroke_event(ev_name);
        }
        ev_handler->add_hook(ev_name, [](const Event* ev, void* user_data) {
            wchar_t keycode = ev->get_parameter(0).get_wstring_value()[0];
            static_cast<Panda3DImGui*>(user_data)->on_keystroke(keycode);
        }, panda3d_imgui_helper);
    }
}

void setup_mouse(WindowFramework* window_framework)
{ // From ImGui helper usage example. Prepare ImGui helper for working with mouse.
    window_framework->enable_keyboard();
    auto mouse_watcher = window_framework->get_mouse();
    auto mouse_watcher_node = DCAST(MouseWatcher, mouse_watcher.node());
    DCAST(PGTop, window_framework->get_pixel_2d().node())->set_mouse_watcher(mouse_watcher_node);
}

Scene* scene;
bool showCShapes = true;
bool CtrlCJustClicked = false;
bool CtrlVJustClicked = false;
bool CtrlXJustClicked = false;
PT(Texture) arrtex;
PT(Texture) crltex;
PT(Texture) szrtex;
ImFont* font;
std::unordered_map<std::string, std::string> loc;
Filename SavePath;

void CtrlC (const Event* event, void* data) {
	CtrlCJustClicked = true;
}
void CtrlV (const Event* event, void* data) {
	CtrlVJustClicked = true;
}
void CtrlX (const Event* event, void* data) {
	CtrlXJustClicked = true;
}

void loadLoc(Filename file) { // Load localisation file.
	std::ifstream stream (file.to_os_specific());
	std::string line;
	while (getline(stream, line, '=')) {
		std::string line2;
		getline (stream, line2);
		loc[line] = line2;
	}
}

void Deselect (NodePath& np) { // Deselect picked node to stop user's interactions with it.
	if (scene->pcdObjs.find(np.node()) == scene->pcdObjs.end()) {
		np.clear_tag("sel");
	}
	for (long int i = 0; i < np.get_num_children(); i++) {
		NodePath child = np.get_child(i);
		if (child.has_tag("disp")) {
			Deselect (child);
		}
	}
}

bool hasDisplayedChildren (const NodePath& np) { // Check if node has displayed children.
	bool has = false;
	for (long int i = 0; i < np.get_num_children(); i++) {
		if (np.get_child(i).has_tag("disp")) {
			has = true;
		}
	}
	return has;
}
void SaveScene(const Filename& path) { // Internal function that launches scene saving.
	std::ofstream strm (path);
	if (strm.is_open()) {
		saveNP(strm, scene->mdroot, "");
		strm.close();
	}
	else {
		return;
	}
}
void SaveAs () { // Save scene to file with custom path.
	nfdchar_t* path = nullptr;
	nfdresult_t res = NFD_SaveDialog(u8"pescn", nullptr, &path);
	if (res == NFD_OKAY) {
		SavePath = Filename::from_os_specific(path);
		if (SavePath.to_os_specific().size() >= 6) {
			if (SavePath.to_os_specific().substr(SavePath.to_os_specific().size() - 6) != u8".pescn") {
				SavePath += u8".pescn";
			}
		}
		SaveScene (SavePath);
	}
	free(path);
}

void Save () { // Is called when user wants to save scene.
	if (SavePath.exists()) {
		SaveScene(SavePath);
	}
	else {
		SaveAs();
	}
}

// Loades scene from editor file.
void Load () {
	nfdchar_t* path = nullptr;
	nfdresult_t res = NFD_OpenDialog("pescn", nullptr, &path);
	if (res == NFD_OKAY) {
		std::ifstream strm(path);
		free (path);
		if (strm.is_open()) {
			while (checkNP(strm)) {
				loadNP(strm, scene->mdroot);
			}
			strm.close();
		}
	}
	else {
		free(path); // Free allocated memory.
	}
}

std::string getCorrName(const NodePath& np) { // Get correct name for NodePath (in this editor, names must be unique). If nodes have the same names, it appends a number to name.
	std::string nm = np.get_name();
	std::unordered_set<int> nums;
	while (nm.size() > 0 && std::isdigit(nm[nm.size()-1], std::locale::global(std::locale("")))) {
		nm.erase(nm.size()-1);
	}
	for (unsigned long int i = 0; i < nm.size(); i++) {
		if (nm[i] == ' ') {
			nm.erase(i, 1);
			i--;
		}
	}
	const NodePath par = np.get_parent();
	for (unsigned int i = 0; i < par.get_num_children(); i++) {
		if (par.get_child(i).node() != np.node()) {
			std::string nm2 = par.get_child(i).get_name();
			unsigned int num10 = 1;
			unsigned int num2 = 0;
			while (nm2.size() > 0 && std::isdigit(nm2[nm2.size()-1], std::locale::global(std::locale("")))) {
				int num0 = nm2[nm2.size() -1] - '0';
				nm2.erase(nm2.size()-1);
				num2 += num0 * num10;
				num10 *= 10;
			}
			if (nm2 == nm) {
				nums.insert(num2);
			}
		}
	}
	unsigned long int num = 0;
	for (; num < ULONG_MAX && (nums.find(num) != nums.end()); num++) {}
	return nm + (num == 0 ? "" : std::to_string(num));
}

inline void setMaskTags(NodePath& np, CollisionNode* nd) { // Set tags for from collision mask (they are used in the export).
	BitMask32 im = nd->get_into_collide_mask();
	BitMask32 fm = nd->get_from_collide_mask();
	for (short int i = 0; i < 32; i++) {
		np.set_tag("im" + std::to_string(i), im.get_bit(i) ? "t" : "f");
		np.set_tag("fm" + std::to_string(i), fm.get_bit(i) ? "t" : "f");
	}
}
inline void setMaskTagsIm(NodePath& np) { // Set tags for into collision mask (they are used in the export).
	BitMask32 im = CollisionNode("nd").get_into_collide_mask();
	for (short int i = 0; i < 32; i++) {
		np.set_tag("im" + std::to_string(i), im.get_bit(i) ? "t" : "f");
	}
}
/*
// Callback that is used in ImGui text input to interact with clipboard.
int ClipboardCallback(ImGuiInputTextCallbackData *data) {
	if (CtrlVJustClicked && clip::has(clip::text_format())) {
		data->BufDirty = true;
		/*if (data->HasSelection()) {
			data->DeleteChars(data->SelectionStart, data->SelectionEnd - data->SelectionStart);
			data->BufTextLen -= data->SelectionEnd - data->SelectionStart;
		}* /
		std::string clipboard_val;
		clip::get_text(clipboard_val);
		data->InsertChars(data->CursorPos, clipboard_val.c_str());
		//data->BufTextLen += clipboard_val.size();
	}
	/*else if (CtrlCJustClicked) {
		std::cout << "CtrlC\n";
		if (data->HasSelection()) {
			clip::set_text(std::string(data->Buf + data->SelectionStart, data->SelectionEnd - data->SelectionStart));data;
		}
	}
	else if (CtrlXJustClicked) {
		std::cout << "CtrlX\n";
		//if (data->HasSelection()) {
			//clip::set_text(std::string(data->Buf + data->SelectionStart, data->SelectionEnd - data->SelectionStart));
			//data->BufDirty = true;
			//data->DeleteChars(data->SelectionStart, data->SelectionEnd - data->SelectionStart);
		//}
	}* /
	return 0;
}*/

void DrawNP (NodePath& np) { // Displays NodePath in the scene graph (tree).
	bool pcd = (scene->isPicked && np.node() == scene->pcdObj.node());
	if (pcd && !np.has_tag("sel")) {
		Deselect(scene->mdroot);
		np.set_tag("sel", "y");
	}
	if (scene->pcdObjs.find(np.node()) != scene->pcdObjs.end()) {
		np.set_tag("sel", "y");
	}
	else if (!pcd) {
		np.clear_tag("sel");
	}
	ImGuiTreeNodeFlags flg = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
	if (np.has_tag("sel")) { // If selected.
		flg |= ImGuiTreeNodeFlags_Selected;
	}
	if (np.has_tag("op")) { // If open.
		flg |= ImGuiTreeNodeFlags_DefaultOpen;
	}
	if (!hasDisplayedChildren(np)) { // If empty (doesn't have displayed childrens).
		flg |= ImGuiTreeNodeFlags_Leaf;
	}

	bool isOpen = ImGui::TreeNodeEx(np.node(), flg, "%s", np.get_name().c_str()); // Create TreeNode.
    if (ImGui::BeginPopupContextItem())
    {
    	if (ImGui::MenuItem(loc[u8"attach_nd"].c_str())) {
    		PT(ModelRoot) node = new ModelRoot("NewNode");
    		NodePath np2 = np.attach_new_node(node);
    		np2.reparent_to(np);
    		np2.set_name(getCorrName(np2));
			np2.set_tag("disp", "y");
			np2.set_tag("exp", "y");
			np2.set_tag("obj", "n"); // It is an attached node.

    	}
    	if (ImGui::BeginMenu(loc[u8"attach_sol"].c_str())) {
    		if (ImGui::MenuItem(loc[u8"solid_sphere"].c_str())) {
    			PT(CollisionSphere) solid = new CollisionSphere(0.0f, 0.0f, 0.0f, 1.0f);
    			PT(CollisionNode) node = new CollisionNode ("Sphere");
    			node->add_solid(solid);
        		NodePath np2 = np.attach_new_node(node);
        		setMaskTags(np2, node);
        		np2.set_name(getCorrName(np2));
    			np2.set_tag("disp", "y");
    			np2.set_tag("exp", "y");
    			np2.set_tag("obj", "s"); // It is a collision node
    			if (showCShapes) {
    				np2.show();
    				np2.set_tag("shs", "y");
    				node->set_into_collide_mask(BitMask32::bit(1));
    			}
    			else {
    				np2.hide();
    				node->set_into_collide_mask(BitMask32::all_off());
    			}
    		}
    		if (ImGui::MenuItem(loc[u8"solid_capsule"].c_str())) {
    			PT(CollisionCapsule) solid = new CollisionCapsule(LPoint3f(0.0f, 0.0f, -1.0f), LPoint3f(0.0f, 0.0f, 1.0f), 1.0f);
    			PT(CollisionNode) node = new CollisionNode ("Capsule");
    			node->add_solid(solid);
        		NodePath np2 = np.attach_new_node(node);
        		setMaskTags(np2, node);
        		np2.set_name(getCorrName(np2));
    			np2.set_tag("disp", "y");
    			np2.set_tag("exp", "y");
    			np2.set_tag("obj", "s");
    			if (showCShapes) {
    				np2.show();
    				np2.set_tag("shs", "y");
    				node->set_into_collide_mask(BitMask32::bit(1));
    			}
    			else {
    				np2.hide();
    				node->set_into_collide_mask(BitMask32::all_off());
    			}
    		}
    		if (ImGui::MenuItem(loc[u8"solid_invsphere"].c_str())) {
    			PT(CollisionInvSphere) solid = new CollisionInvSphere(0.0f, 0.0f, 0.0f, 1.0f);
    			PT(CollisionNode) node = new CollisionNode ("Inverted sphere");
    			node->add_solid(solid);
        		NodePath np2 = np.attach_new_node(node);
        		setMaskTags(np2, node);
        		np2.set_name(getCorrName(np2));
    			np2.set_tag("disp", "y");
    			np2.set_tag("exp", "y");
    			np2.set_tag("obj", "s");
    			if (showCShapes) {
    				np2.show();
    				np2.set_tag("shs", "y");
    				node->set_into_collide_mask(BitMask32::bit(1));
    			}
    			else {
    				np2.hide();
    				node->set_into_collide_mask(BitMask32::all_off());
    			}
    		}
    		if (ImGui::MenuItem(loc[u8"solid_plane"].c_str())) {
    			PT(CollisionPlane) solid = new CollisionPlane(LPlanef(LVector3f(0.0f, 0.0f, 1.0f), LPoint3f(0.0f, 0.0f, 0.0f)));
    			PT(CollisionNode) node = new CollisionNode ("Plane");
    			node->add_solid(solid);
        		NodePath np2 = np.attach_new_node(node);
        		setMaskTags(np2, node);
        		np2.set_name(getCorrName(np2));
    			np2.set_tag("disp", "y");
    			np2.set_tag("exp", "y");
    			np2.set_tag("obj", "s");
    			if (showCShapes) {
    				np2.show();
    				np2.set_tag("shs", "y");
    				node->set_into_collide_mask(BitMask32::bit(1));
    			}
    			else {
    				np2.hide();
    				node->set_into_collide_mask(BitMask32::all_off());
    			}
    		}
    		if (ImGui::MenuItem(loc[u8"solid_polygon"].c_str())) {
    			PT(CollisionPolygon) solid = new CollisionPolygon(LPoint3f(0.0f, 0.0f, 0.0f), LPoint3f(2.0f, 0.0f, 0.0f), LPoint3f(1.0f, 2.0f, 0.0f));
    			PT(CollisionNode) node = new CollisionNode ("Polygon");
    			node->add_solid(solid);
        		NodePath np2 = np.attach_new_node(node);
        		setMaskTags(np2, node);
        		np2.set_name(getCorrName(np2));
    			np2.set_tag("disp", "y");
    			np2.set_tag("exp", "y");
    			np2.set_tag("obj", "s"); // It is an attached collision solid.
    			if (showCShapes) {
    				np2.show();
    				np2.set_tag("shs", "y");
    				node->set_into_collide_mask(BitMask32::bit(1));
    			}
    			else {
    				np2.hide();
    				node->set_into_collide_mask(BitMask32::all_off());
    			}
    		}
    		if (ImGui::MenuItem(loc[u8"solid_box"].c_str())) {
    			PT(CollisionBox) solid = new CollisionBox(LPoint3f(0.0f, 0.0f, 0.0f), 1.0f, 1.0f, 1.0f);
    			PT(CollisionNode) node = new CollisionNode ("Box");
    			node->add_solid(solid);
        		NodePath np2 = np.attach_new_node(node);
        		setMaskTags(np2, node);
        		np2.set_name(getCorrName(np2));
    			np2.set_tag("disp", "y");
    			np2.set_tag("exp", "y");
    			np2.set_tag("obj", "s");
    			if (showCShapes) {
    				np2.show();
    				np2.set_tag("shs", "y");
    				node->set_into_collide_mask(BitMask32::bit(1));
    			}
    			else {
    				np2.hide();
    				node->set_into_collide_mask(BitMask32::all_off());
    			}
    		}
    		ImGui::EndMenu();
    	}
        ImGui::EndPopup();
    }
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
		if (mWatcher->is_button_down(KeyboardButton::shift())) {
			np.set_tag("sel", "y");
			scene->pickAddObj(np);
		}
		else {
			Deselect(scene->mdroot); // Deselect all nodes
			np.set_tag("sel", "y"); // Select this node
			scene->pickObj(np);
		}
	}
	if (ImGui::BeginDragDropSource()) { // If node is dragged
		PandaNode* nd = np.node();
		ImGui::SetDragDropPayload("NODE", &nd, sizeof(void*));
		ImGui::EndDragDropSource();
	}
	if (ImGui::BeginDragDropTarget ()) {
		 if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("NODE")) {
			 if(payload->DataSize == sizeof(void*)) {
				PandaNode* nd = *(PandaNode**)payload->Data;
				NodePath dobj (nd);
				//dobj.set_pos(dobj.get_pos(np));
				dobj.reparent_to(np);
				dobj.set_name(getCorrName(dobj));
			 }
		 }
		 ImGui::EndDragDropTarget();
	}
	if (isOpen) {
		for (long int i = 0; i < np.get_num_children(); i++) {
			NodePath child = np.get_child(i);
			if (child.has_tag("disp")) {
				DrawNP(child);
			}
		}
		ImGui::TreePop();
		np.set_tag("op", "y");
	}
	else {
		np.clear_tag("op");
	}
}

void ImGuiNewFrame () { // Is called every frame to draw all the UI.
	//static bool first = true;
	//if (first) {
	//	font = ImGui::GetIO().Fonts->AddFontFromFileTTF("/usr/share/fonts/timesnewroman/timesnewromanpsmt.ttf", 23.0f, NULL, ImGui::GetIO().Fonts->GetGlyphRangesDefault());
    //	IM_ASSERT(font!= NULL);
	//}
	//static bool show_sett_w = false;
	static bool show_mod_conf = false;
	float mby = 0;
	bool noActions = false; // Used to make Scene not eact to keys: Delete and Arrows
	bool isHovered = false;
	// Menu bar
	if (ImGui::BeginMainMenuBar()) {
			mby = ImGui::GetWindowHeight();
	    	if (ImGui::BeginMenu(loc[u8"menu_file"].c_str())) {
	    		if (ImGui::MenuItem (loc[u8"menu_act_save"].c_str())) {
	    			Save();
	    		}
	    		if (ImGui::MenuItem(loc[u8"menu_act_save_as"].c_str())) {
	    			SaveAs();
	    		}
	    		if (ImGui::MenuItem(loc[u8"menu_act_load_scene"].c_str())) {
	    			Load();
	    		}
	    		if (ImGui::MenuItem(loc[u8"menu_act_load"].c_str())) {
	    			//show_l_w = true;
	    			nfdchar_t* path = nullptr;
	    			nfdresult_t res = NFD_OpenDialog("bam,glb", nullptr, &path);
	    			if (res == NFD_OKAY) {
	    				Filename fname (path);
	    				NodePath model = window->load_model(scene->mdroot, path);
	    				model.set_name(fname.get_basename_wo_extension());
	    				model.set_collide_mask(BitMask32::bit(1));
	    				model.set_tag("disp", "y");
	    				model.set_tag("exp", "y");
	    				model.set_tag("path", fname.to_os_specific());
	    				model.set_tag("obj", "m"); // It is a loaded model.
	    				model.set_name(getCorrName(model));
	    			}
	    			free(path); // Free allocated memory.
	    		}
	    		if (ImGui::BeginMenu(loc[u8"menu_act_export"].c_str())) {
	    			if (ImGui::MenuItem(loc[u8"menu_act_export_cpp_macro"].c_str())) {
		    			nfdchar_t* path = nullptr;
		    			nfdresult_t res = NFD_SaveDialog("h,hpp", nullptr, &path);
		    			if (res == NFD_OKAY) {
		    				ExportCppMacro(Filename(path));
		    			}
		    			free(path);
	    			}
	    			if (ImGui::MenuItem(loc[u8"menu_act_export_py"].c_str())) {
		    			nfdchar_t* path = nullptr;
		    			nfdresult_t res = NFD_SaveDialog("py", nullptr, &path);
		    			if (res == NFD_OKAY) {
		    				ExportPy(Filename(path));
		    			}
		    			free(path);
	    			}
	    			ImGui::EndMenu();
	    		}
	    		ImGui::EndMenu();
	        }
	    	if (ImGui::BeginMenu(loc["menu_view"].c_str())) {
	    		ImGui::MenuItem(loc["menu_show_cs"].c_str(), "", &showCShapes);
	    		ImGui::EndMenu();
	    	}
	    	isHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenOverlapped) ? true : isHovered;
	    	ImGui::EndMainMenuBar();
	}
	ImGui::SetNextWindowPos(ImVec2(-1.0f, mby));
	ImGui::SetNextWindowSize(ImVec2(160, window->get_graphics_window()->get_properties().get_y_size() - mby));
	static const char* items[] = { loc[u8"or_global"].c_str(), loc[u8"or_local"].c_str()};
	static const char* current_item = items[0];
	if (ImGui::Begin("Tools", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground)) {
		ImGui::Text("%s",loc[u8"orientation"].c_str());
		if (ImGui::BeginCombo(" ", current_item)) // The second parameter is the label previewed before opening the combo.
		{
		    for (int n = 0; n < IM_ARRAYSIZE(items); n++)
		    {
		        bool is_selected = (current_item == items[n]); // You can store your selection however you want, outside or inside your objects
		        if (ImGui::Selectable(items[n], is_selected)) {
		            current_item = items[n];
		            scene->SetOrientation(!n);
		        }
		        isHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenOverlapped) ? true : isHovered;
		        if (is_selected) {
		            ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
		        }
		    }
		    ImGui::EndCombo();
		}
		isHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenOverlapped) ? true : isHovered;
		if (arrtex && arrtex->is_prepared(window->get_graphics_window()->get_gsg()->get_prepared_objects())) {
			float coladd = (scene->GetTool() == 0 ? 0.3f : 0.0f);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.25f + coladd));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.5f + coladd));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.35f + coladd));
			if (ImGui::ImageButton("arrow_button",reinterpret_cast<ImTextureID>(arrtex.p()), ImVec2(50, 50), ImVec2 (0.0f, 0.0f), ImVec2(1.0f, 1.0f))) {
				scene->SetTool(0);
			}
			ImGui::PopStyleColor(3);
		}
		isHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenOverlapped) ? true : isHovered;
		if (crltex && crltex->is_prepared(window->get_graphics_window()->get_gsg()->get_prepared_objects())) {
			float coladd = (scene->GetTool() == 1 ? 0.3f : 0.0f);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.25f + coladd));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.5f + coladd));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.35f + coladd));
			if (ImGui::ImageButton("circle_button",reinterpret_cast<ImTextureID>(crltex.p()), ImVec2(50, 50), ImVec2 (0.0f, 0.0f), ImVec2(1.0f, 1.0f))) {
				scene->SetTool(1);
			}
			ImGui::PopStyleColor(3);
		}
		isHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenOverlapped) ? true : isHovered;
		if (szrtex && szrtex->is_prepared(window->get_graphics_window()->get_gsg()->get_prepared_objects())) {
			float coladd = (scene->GetTool() == 2 ? 0.3f : 0.0f);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.25f + coladd));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.5f + coladd));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.35f + coladd));
			if (ImGui::ImageButton("sizer_button",reinterpret_cast<ImTextureID>(szrtex.p()), ImVec2(50, 50), ImVec2 (0.0f, 0.0f), ImVec2(1.0f, 1.0f))) {
				scene->SetTool(2);
			}
			ImGui::PopStyleColor(3);
		}
		isHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenOverlapped) ? true : isHovered;
		ImGui::End();
		ImVec2 size (0, window->get_graphics_window()->get_properties().get_y_size()/3.0f);
		size.x = size.y / 1.5f;
		ImGui::SetNextWindowPos(ImVec2(window->get_graphics_window()->get_properties().get_x_size() - size.x + 1.0f, mby - 1.0f));
		ImGui::SetNextWindowSize(size);
		if (ImGui::Begin(loc[u8"window_scene"].c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
			DrawNP(scene->mdroot);
			isHovered = ImGui::IsWindowHovered() ? true : isHovered;
			ImGui::End();
		}
		ImGui::SetNextWindowPos(ImVec2(window->get_graphics_window()->get_properties().get_x_size() - size.x + 1.0f, mby + size.y - 1.0f));
		size.y = window->get_graphics_window()->get_properties().get_y_size()/1.5f;
		ImGui::SetNextWindowSize(size);
		if (ImGui::Begin(loc[u8"window_obj"].c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
			if (scene->isPicked) {
				char buf[50];
				std::string str = scene->pcdObj.get_name().c_str();
				for (int i = 0; i < 50; i++) {
					buf[i] = (i < str.size() ? str[i] : '\0');
				}
				if (ImGui::InputText(loc[u8"name"].c_str(), buf, 50)) {
					scene->pcdObj.set_name(buf);
					scene->pcdObj.set_name(getCorrName(scene->pcdObj));
				}
				if (ImGui::IsItemFocused()) {
					scene->noKeysActs = true;
					noActions = true;
				}
				if (scene->pcdObj.node()->is_of_type(CollisionNode::get_class_type())) {
					bool show = scene->pcdObj.has_tag("shs");
					if (ImGui::Checkbox(loc[u8"show"].c_str(), &show)) {
						if (show) {
							scene->pcdObj.show();
							scene->pcdObj.set_tag("shs", "y");
							scene->pcdObj.node()->set_into_collide_mask(BitMask32::bit(1));
						}
						else {
							scene->pcdObj.hide();
							scene->pcdObj.clear_tag("shs");
							scene->pcdObj.node()->set_into_collide_mask(BitMask32::all_off());
						}
					}
				}
				else {
					bool show = !scene->pcdObj.is_hidden();
					if (ImGui::Checkbox(loc[u8"show"].c_str(), &show)) {
						if (show) {
							scene->pcdObj.show();
						}
						else {
							scene->pcdObj.hide();
						}
					}
				}
				float x = scene->pcdObj.get_x(), y = scene->pcdObj.get_y(), z = scene->pcdObj.get_z();
				ImGui::Text("%s",loc[u8"position"].c_str());
				if (ImGui::InputFloat("X", &x)) {
					scene->pcdObj.set_x(scene->rcroot, x);
					scene->SetTool(scene->GetTool());
				}
				if (ImGui::IsItemFocused()) {
					scene->noKeysActs = true;
					noActions = true;
				}
				if (ImGui::InputFloat("Y", &y)) {
					scene->pcdObj.set_y(scene->rcroot, y);
					scene->SetTool(scene->GetTool());
				}
				if (ImGui::IsItemFocused()) {
					scene->noKeysActs = true;
					noActions = true;
				}
				if (ImGui::InputFloat("Z", &z)) {
					scene->pcdObj.set_z(scene->rcroot, z);
					scene->SetTool(scene->GetTool());
				}
				if (ImGui::IsItemFocused()) {
					scene->noKeysActs = true;
					noActions = true;
				}
				ImGui::Spacing();
				float x1 = scene->pcdObj.get_h(), y1 = scene->pcdObj.get_p(), z1 = scene->pcdObj.get_r();
				ImGui::Text("%s",loc[u8"angle"].c_str());
				if (ImGui::InputFloat("H", &x1)) {
					scene->pcdObj.set_h(scene->rcroot, x1);
					scene->SetTool(scene->GetTool());
				}
				if (ImGui::IsItemFocused()) {
					scene->noKeysActs = true;
					noActions = true;
				}
				if (ImGui::InputFloat("P", &y1)) {
					scene->pcdObj.set_p(scene->rcroot, y1);
					scene->SetTool(scene->GetTool());
				}
				if (ImGui::IsItemFocused()) {
					scene->noKeysActs = true;
					noActions = true;
				}
				if (ImGui::InputFloat("R", &z1)) {
					scene->pcdObj.set_r(scene->rcroot, z1);
					scene->SetTool(scene->GetTool());
				}
				if (ImGui::IsItemFocused()) {
					scene->noKeysActs = true;
					noActions = true;
				}
				ImGui::Spacing();
				float x2 = scene->pcdObj.get_scale().get_x(), y2 = scene->pcdObj.get_scale().get_y(), z2 = scene->pcdObj.get_scale().get_z();
				ImGui::Text("%s",loc[u8"scale"].c_str());
				if (ImGui::InputFloat("X##x2", &x2)) {
					scene->pcdObj.set_scale(scene->rcroot, x2, y2, z2);
					scene->SetTool(scene->GetTool());
				}
				if (ImGui::IsItemFocused()) {
					scene->noKeysActs = true;
					noActions = true;
				}
				if (ImGui::InputFloat("Y##y2", &y2)) {
					scene->pcdObj.set_scale(scene->rcroot, x2, y2, z2);
					scene->SetTool(scene->GetTool());
				}
				if (ImGui::IsItemFocused()) {
					scene->noKeysActs = true;
					noActions = true;
				}
				if (ImGui::InputFloat("Z##z2", &z2)) {
					scene->pcdObj.set_scale(scene->rcroot, x2, y2, z2);
					scene->SetTool(scene->GetTool());
				}
				if (ImGui::IsItemFocused()) {
					scene->noKeysActs = true;
					noActions = true;
				}
				if (ImGui::Button(loc[u8"advanced"].c_str())) {
					show_mod_conf = true;
				}
			}
			isHovered = ImGui::IsWindowHovered() ? true : isHovered;
			ImGui::End();
		}
		//static int cpplen = std::max<int>(scene->pcdObj.get_tag("cpp_code").size(), 300);
		//static int pylen = std::max<int>(scene->pcdObj.get_tag("python_code").size(), 300);
		if (show_mod_conf) {
			if (!scene->pcdObj.has_tag("im0")) {
				setMaskTagsIm(scene->pcdObj);
			}
			if (ImGui::Begin(loc[u8"advanced"].c_str(), &show_mod_conf)) {
				if (ImGui::BeginTabBar("TBar", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton)) {
					if (ImGui::BeginTabItem(loc[u8"tab_general"].c_str())) {
						bool transp = scene->pcdObj.has_tag("transp");
						if (ImGui::Checkbox(loc[u8"transparency"].c_str(), &transp)) {
							if (transp) {
								scene->pcdObj.set_tag("transp", "y");
								scene->pcdObj.set_transparency(TransparencyAttrib::M_alpha);
							} else {
								scene->pcdObj.set_transparency(TransparencyAttrib::M_none);
								scene->pcdObj.clear_tag("transp");
							}
						}
						if (transp) {
							float val = scene->pcdObj.get_color_scale().get_w();
							if (ImGui::InputFloat(loc[u8"alpha"].c_str(), &val)) {
								scene->pcdObj.set_alpha_scale(val);
							}
						}
						if (scene->pcdObj.node()->is_collision_node()) {
							CollisionNode* node = DCAST(CollisionNode, scene->pcdObj.node());
							ImGui::Text(loc[u8"imask"].c_str());
							if (ImGui::BeginTable("icmask", 8)) {
								for (short int row = 0; row < 4; row++) {
									ImGui::TableNextRow();
									for (short int col = 0; col < 8; col++) {
										ImGui::TableSetColumnIndex(col);
										bool cb = (scene->pcdObj.get_tag("im" + std::to_string(row * 8 + col)) == "t") ? true : false;
										if (ImGui::Checkbox(std::to_string(row * 8 + col).c_str(), &cb)) {
											scene->pcdObj.set_tag("im" + std::to_string(row * 8 + col), cb ? "t" : "f");
										}
									}
								}
								ImGui::EndTable();
							}
							ImGui::Text(loc[u8"fmask"].c_str());
							if (ImGui::BeginTable("fcmask", 8)) {
								for (short int row = 0; row < 4; row++) {
									ImGui::TableNextRow();
									for (short int col = 0; col < 8; col++) {
										ImGui::TableSetColumnIndex(col);
										bool cb = (scene->pcdObj.get_tag("fm" + std::to_string(row * 8 + col)) == "t") ? true : false;
										if (ImGui::Checkbox(std::to_string(row * 8 + col).c_str(), &cb)) {
											scene->pcdObj.set_tag("fm" + std::to_string(row * 8 + col), cb ? "t" : "f");
										}
									}
								}
								ImGui::EndTable();
							}
							static bool isOpen;
							isOpen = scene->pcdObj.has_tag("solids_open");
							static std::vector<std::pair<bool, bool>> nodeVec{}; // bool: open, selected.
							ImGuiTreeNodeFlags flg = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
							if (isOpen) {
								flg |= ImGuiTreeNodeFlags_DefaultOpen;
							}
							isOpen = ImGui::TreeNodeEx(loc[u8"solids"].c_str(), flg);
							if (isOpen) {
								bool justOpen = !scene->pcdObj.has_tag("solids_open");
								scene->pcdObj.set_tag("solids_open", "");
						    	if (ImGui::BeginMenu(loc[u8"add_sol"].c_str())) {
						       		if (ImGui::MenuItem(loc[u8"solid_sphere"].c_str())) {
						        		PT(CollisionSphere) solid = new CollisionSphere(0.0f, 0.0f, 0.0f, 1.0f);
						        		node->add_solid(solid);
						        		nodeVec.push_back(std::pair<bool, bool>(false, false));
						        	}
						        	if (ImGui::MenuItem(loc[u8"solid_capsule"].c_str())) {
						        		PT(CollisionCapsule) solid = new CollisionCapsule(LPoint3f(0.0f, 0.0f, -1.0f), LPoint3f(0.0f, 0.0f, 1.0f), 1.0f);
						        		node->add_solid(solid);
						        		nodeVec.push_back(std::pair<bool, bool>(false, false));
						        	}
						        	if (ImGui::MenuItem(loc[u8"solid_invsphere"].c_str())) {
						        		PT(CollisionInvSphere) solid = new CollisionInvSphere(0.0f, 0.0f, 0.0f, 1.0f);
						        		node->add_solid(solid);
						        		nodeVec.push_back(std::pair<bool, bool>(false, false));
						        	}
						        	if (ImGui::MenuItem(loc[u8"solid_plane"].c_str())) {
						        		PT(CollisionPlane) solid = new CollisionPlane(LPlanef(LVector3f(0.0f, 0.0f, 1.0f), LPoint3f(0.0f, 0.0f, 0.0f)));
						        		node->add_solid(solid);
						        		nodeVec.push_back(std::pair<bool, bool>(false, false));
						        	}
						        	if (ImGui::MenuItem(loc[u8"solid_polygon"].c_str())) {
						        		PT(CollisionPolygon) solid = new CollisionPolygon(LPoint3f(0.0f, 0.0f, 0.0f), LPoint3f(2.0f, 0.0f, 0.0f), LPoint3f(1.0f, 2.0f, 0.0f));
						        		node->add_solid(solid);
						        		nodeVec.push_back(std::pair<bool, bool>(false, false));
						        	}
						        	if (ImGui::MenuItem(loc[u8"solid_box"].c_str())) {
						        		PT(CollisionBox) solid = new CollisionBox(LPoint3f(0.0f, 0.0f, 0.0f), 1.0f, 1.0f, 1.0f);
						        		node->add_solid(solid);
						        		nodeVec.push_back(std::pair<bool, bool>(false, false));
						        	}
						    		ImGui::EndMenu();
						    	}
						    	ImGui::SameLine();
						    	ImGui::Spacing();
						    	ImGui::Spacing();
								for (unsigned long int i = 0; i < node->get_num_solids(); i++) {
									if (justOpen) {
										nodeVec.push_back(std::pair<bool, bool>(false, false));
									}
									ImGuiTreeNodeFlags flg = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
									if (nodeVec[i].first) {
										flg |= ImGuiTreeNodeFlags_DefaultOpen;
									}
									if (nodeVec[i].second) {
										flg |= ImGuiTreeNodeFlags_Selected;
									}
									nodeVec[i].first = ImGui::TreeNodeEx((loc[u8"solid"] + " " + std::to_string(i)).c_str(), flg);
									if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
										nodeVec[i].second = !nodeVec[i].second;
									}
									if (mWatcher->is_button_down(KeyboardButton::del()) && nodeVec[i].second == true) {
										node->remove_solid(i);
										nodeVec.erase(nodeVec.begin() + i);
										i--;
									}
									else if (nodeVec[i].first) {
										if (node->get_solid(i)->get_type_index() == CollisionSphere::get_class_type().get_index()) {
											ImGui::Text((loc[u8"solid_sphere"]).c_str());
											const CollisionSphere* spr = static_cast<const CollisionSphere*>(node->get_solid(i).p());
											float cx, cy, cz, rad;
											fromPoint(spr->get_center(), cx, cy, cz);
											rad = spr->get_radius();
											bool ch = ImGui::InputFloat((loc[u8"radius"] + "##" + std::to_string(i)).c_str(), &rad);
											ch = ch || ImGui::InputFloat((loc[u8"x"] + "##" + std::to_string(i)).c_str(), &cx);
											ch = ch || ImGui::InputFloat((loc[u8"y"] + "##" + std::to_string(i)).c_str(), &cy);
											ch = ch || ImGui::InputFloat((loc[u8"z"] + "##" + std::to_string(i)).c_str(), &cz);
											if (ImGui::Button((loc[u8"remove"] + "##" + std::to_string(i)).c_str())) {
												node->remove_solid(i);
												nodeVec.erase(nodeVec.begin() + i);
												i--;
											} else if (ch) {
												PT(CollisionSphere) spr2 = new CollisionSphere(cx, cy, cz, rad);
												node->set_solid(i, spr2);
											}
										} else if (node->get_solid(i)->get_type_index() == CollisionCapsule::get_class_type().get_index()) {
											ImGui::Text(loc[u8"solid_capsule"].c_str());
											const CollisionCapsule* cap = static_cast<const CollisionCapsule*>(node->get_solid(i).p());
											float ax, ay, az, bx, by, bz, rad;
											fromPoint(cap->get_point_a(), ax, ay, az);
											fromPoint(cap->get_point_b(), bx, by, bz);
											rad = cap->get_radius();
											bool ch = ImGui::InputFloat((loc[u8"radius"]  + "##" + std::to_string(i)).c_str(), &rad);
											ch = ch || ImGui::InputFloat((loc[u8"point"] + "1 " + loc[u8"x"] + "##" + std::to_string(i)).c_str(), &ax);
											ch = ch || ImGui::InputFloat((loc[u8"point"] + "1 " + loc[u8"y"] + "##" + std::to_string(i)).c_str(), &ay);
											ch = ch || ImGui::InputFloat((loc[u8"point"] + "1 " + loc[u8"z"] + "##" + std::to_string(i)).c_str(), &az);
											ch = ch || ImGui::InputFloat((loc[u8"point"] + "2 " + loc[u8"x"] + "##" + std::to_string(i)).c_str(), &bx);
											ch = ch || ImGui::InputFloat((loc[u8"point"] + "2 " + loc[u8"y"] + "##" + std::to_string(i)).c_str(), &by);
											ch = ch || ImGui::InputFloat((loc[u8"point"] + "2 " + loc[u8"z"] + "##" + std::to_string(i)).c_str(), &bz);
											if (ImGui::Button((loc[u8"remove"] + "##" + std::to_string(i)).c_str())) {
												node->remove_solid(i);
												nodeVec.erase(nodeVec.begin() + i);
												i--;
											} else if (ch) {
												PT(CollisionCapsule) cap2 = new CollisionCapsule(ax, ay, az, bx, by, bz, rad);
												node->set_solid(i, cap2);
											}
										} else if (node->get_solid(i)->get_type_index() == CollisionInvSphere::get_class_type().get_index()) {
											ImGui::Text(loc[u8"solid_invsphere"].c_str());
											const CollisionInvSphere* spr = static_cast<const CollisionInvSphere*>(node->get_solid(i).p());
											float cx, cy, cz, rad;
											fromPoint(spr->get_center(), cx, cy, cz);
											rad = spr->get_radius();
											bool ch = ImGui::InputFloat((loc[u8"radius"] + "##" + std::to_string(i)).c_str(), &rad);
											ch = ch || ImGui::InputFloat((loc[u8"x"] + "##" + std::to_string(i)).c_str(), &cx);
											ch = ch || ImGui::InputFloat((loc[u8"y"] + "##" + std::to_string(i)).c_str(), &cy);
											ch = ch || ImGui::InputFloat((loc[u8"z"] + "##" + std::to_string(i)).c_str(), &cz);
											if (ImGui::Button((loc[u8"remove"] + "##" + std::to_string(i)).c_str())) {
												node->remove_solid(i);
												nodeVec.erase(nodeVec.begin() + i);
												i--;
											} else if (ch) {
												PT(CollisionInvSphere) spr2 = new CollisionInvSphere(cx, cy, cz, rad);
												node->set_solid(i, spr2);
											}
										} else if (node->get_solid(i)->get_type_index() == CollisionPlane::get_class_type().get_index()) {
											ImGui::Text(loc[u8"solid_plane"].c_str());
											const CollisionPlane* sol = static_cast<const CollisionPlane*>(node->get_solid(i).p());
											float nx, ny, nz, px, py, pz;
											fromPoint(sol->get_plane().get_point(), px, py, pz);
											fromPoint(LPoint3f(sol->get_normal()), nx, ny, nz);
											bool ch = ImGui::InputFloat((loc[u8"point"] + " " + loc[u8"x"] + "##" + std::to_string(i)).c_str(), &px);
											ch = ch || ImGui::InputFloat((loc[u8"point"] + " " + loc[u8"y"] + "##" + std::to_string(i)).c_str(), &py);
											ch = ch || ImGui::InputFloat((loc[u8"point"] + " " + loc[u8"z"] + "##" + std::to_string(i)).c_str(), &pz);
											ch = ch || ImGui::InputFloat((loc[u8"normal"] + " " + loc[u8"x"] + "##" + std::to_string(i)).c_str(), &nx);
											ch = ch || ImGui::InputFloat((loc[u8"normal"] + " " + loc[u8"y"] + "##" + std::to_string(i)).c_str(), &ny);
											ch = ch || ImGui::InputFloat((loc[u8"normal"] + " " + loc[u8"z"] + "##" + std::to_string(i)).c_str(), &nz);
											if (ImGui::Button((loc[u8"remove"] + "##" + std::to_string(i)).c_str())) {
												node->remove_solid(i);
												nodeVec.erase(nodeVec.begin() + i);
											} else if (ch) {
												PT(CollisionPlane) sol2 = new CollisionPlane(LPlanef(LVector3f(nx, ny, nz), LPoint3f(px, py, pz)));
												node->set_solid(i, sol2);
											}
										} else if (node->get_solid(i)->get_type_index() == CollisionPolygon::get_class_type().get_index()) {
											ImGui::Text(loc[u8"solid_polygon"].c_str());
											const CollisionPolygon* sol = static_cast<const CollisionPolygon*>(node->get_solid(i).p());
											float x1, y1, z1, x2, y2, z2, x3, y3, z3;
											fromPoint(sol->get_point(0), x1, y1, z1);
											fromPoint(sol->get_point(1), x2, y2, z2);
											fromPoint(sol->get_point(2), x3, y3, z3);
											bool ch = ImGui::InputFloat((loc[u8"point"] + " 1 " + loc[u8"x"]  + "##" + std::to_string(i)).c_str(), &x1);
											ch = ch || ImGui::InputFloat((loc[u8"point"] + " 1 " + loc[u8"y"] + "##" + std::to_string(i)).c_str(), &y1);
											ch = ch || ImGui::InputFloat((loc[u8"point"] + " 1 " + loc[u8"z"] + "##" + std::to_string(i)).c_str(), &z1);
											ch = ch || ImGui::InputFloat((loc[u8"point"] + " 2 " + loc[u8"x"] + "##" + std::to_string(i)).c_str(), &x2);
											ch = ch || ImGui::InputFloat((loc[u8"point"] + " 2 " + loc[u8"y"] + "##" + std::to_string(i)).c_str(), &y2);
											ch = ch || ImGui::InputFloat((loc[u8"point"] + " 2 " + loc[u8"z"] + "##" + std::to_string(i)).c_str(), &z2);
											ch = ch || ImGui::InputFloat((loc[u8"point"] + " 3 " + loc[u8"x"] + "##" + std::to_string(i)).c_str(), &x3);
											ch = ch || ImGui::InputFloat((loc[u8"point"] + " 3 " + loc[u8"y"] + "##" + std::to_string(i)).c_str(), &y3);
											ch = ch || ImGui::InputFloat((loc[u8"point"] + " 3 " + loc[u8"z"] + "##" + std::to_string(i)).c_str(), &z3);
											if (ImGui::Button((loc[u8"remove"] + "##" + std::to_string(i)).c_str())) {
												node->remove_solid(i);
												nodeVec.erase(nodeVec.begin() + i);
											} else if (ch) {
												PT(CollisionPolygon) sol2 = new CollisionPolygon(LPoint3f(x1, y1, z1), LPoint3f(x2, y2, z2), LPoint3f(x3, y3, z3));
												node->set_solid(i, sol2);
											}
										} else if (node->get_solid(i)->get_type_index() == CollisionBox::get_class_type().get_index()) {
											ImGui::Text(loc[u8"solid_box"].c_str());
											const CollisionBox* sol = static_cast<const CollisionBox*>(node->get_solid(i).p());
											float x1, y1, z1, x2, y2, z2;
											fromPoint(sol->get_center(), x1, y1, z1);
											fromPoint(sol->get_dimensions() / 2, x2, y2, z2);
											bool ch = ImGui::InputFloat((loc[u8"x"] + "##" + std::to_string(i)).c_str(), &x1);
											ch = ch || ImGui::InputFloat((loc[u8"y"] + "##" + std::to_string(i)).c_str(), &y1);
											ch = ch || ImGui::InputFloat((loc[u8"z"] + "##" + std::to_string(i)).c_str(), &z1);
											ch = ch || ImGui::InputFloat((loc[u8"dist"] + " " + loc[u8"x"] + "##" + std::to_string(i)).c_str(), &x2);
											ch = ch || ImGui::InputFloat((loc[u8"dist"] + " " + loc[u8"y"] + "##" + std::to_string(i)).c_str(), &y2);
											ch = ch || ImGui::InputFloat((loc[u8"dist"] + " " + loc[u8"z"] + "##" + std::to_string(i)).c_str(), &z2);
											if (ImGui::Button((loc[u8"remove"] + "##" + std::to_string(i)).c_str())) {
												node->remove_solid(i);
												nodeVec.erase(nodeVec.begin() + i);
												i--;
											} else if (ch) {
												PT(CollisionBox) sol2 = new CollisionBox(LPoint3f(x1, y1, z1), x2, y2, z2);
												node->set_solid(i, sol2);
											}
										}
										ImGui::TreePop();
									}
								}
								ImGui::TreePop();
							}
							else {
								scene->pcdObj.clear_tag("solids_open");
								nodeVec.clear();
							}
						}
						else { // If it isn't collision node
							bool hasCM = scene->pcdObj.has_tag("cm");
							ImGui::Checkbox(loc[u8"cmask"].c_str(), &hasCM);
							if (hasCM) {
								scene->pcdObj.set_tag("cm", "y");
								ImGui::Text(loc[u8"imask"].c_str());
								if (ImGui::BeginTable("icmask", 8)) {
									for (short int row = 0; row < 4; row++) {
										ImGui::TableNextRow();
										for (short int col = 0; col < 8; col++) {
											ImGui::TableSetColumnIndex(col);
											bool cb = (scene->pcdObj.get_tag("im" + std::to_string(row * 8 + col)) == "t") ? true : false;
											if (ImGui::Checkbox(std::to_string(row * 8 + col).c_str(), &cb)) {
												scene->pcdObj.set_tag("im" + std::to_string(row * 8 + col), cb ? "t" : "f");
											}
										}
									}
									ImGui::EndTable();
								}
							}
							else {
								scene->pcdObj.clear_tag("cm");
							}
						}
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem(loc[u8"tab_cpp"].c_str())) {
						std::string code = scene->pcdObj.get_tag("cpp_code");
						ImGui::Text((loc["label_file"] + (code.size() > 0 ? code.c_str() : loc["label_no_file"])).c_str());
						if (ImGui::Button(loc["select_file"].c_str())) {
							nfdchar_t* path = nullptr;
							nfdresult_t res = NFD_OpenDialog("cpp;txt;*", nullptr, &path);
							if (res == NFD_OKAY) {
								scene->pcdObj.set_tag("cpp_code", path);
							}
							free(path); // Free allocated memory.
						}
						//const unsigned long int clipboard_size = CtrlVJustClicked? clip::get_text();
						/*int len2 = std::max<int>(cpplen, code.size() * 1.5);
						char* buf = new char[len2];
						for (unsigned int i = 0; i < len2; i++) {
							buf[i] = '\0';
						}
						for (unsigned int i = 0; i < code.size() && i < std::max(len2, 100); i++) {
							buf[i] = code[i];
						}
						ImVec2 sz = ImGui::GetWindowSize(); sz.x -= sz.x/5; sz.y -= sz.y/10;
						if (ImGui::InputTextMultiline(loc[u8"custom_code"].c_str(), buf, len2, sz, ImGuiInputTextFlags_CallbackAlways, &ClipboardCallback)) {
							cpplen = (strlen(buf) + 10) * 1.5;
							scene->pcdObj.set_tag("cpp_code", buf);
						}
						delete[] buf;*/
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem(loc[u8"tab_python"].c_str())) {
						std::string code = scene->pcdObj.get_tag("python_code");
						ImGui::Text((loc["label_file"] + (code.size() > 0 ? code : loc["label_no_file"])).c_str());
						if (ImGui::Button(loc["select_file"].c_str())) {
							nfdchar_t* path = nullptr;
							nfdresult_t res = NFD_OpenDialog("py;txt;*", nullptr, &path);
							if (res == NFD_OKAY) {
								scene->pcdObj.set_tag("python_code", path);
							}
							free(path); // Free allocated memory.
						}
						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}
				isHovered = true;
				scene->noKeysActs = true;
				noActions = true;
			}
			ImGui::End();
		}
		else {
			//cpplen = std::max<int>(scene->pcdObj.get_tag("cpp_code").size(), 300);
			//pylen = std::max<int>(scene->pcdObj.get_tag("python_code").size(), 300);
		}
		scene->setDNPick(isHovered);
		scene->noKeysActs = noActions;
	}
	CtrlCJustClicked = false;
	CtrlVJustClicked = false;
	CtrlXJustClicked = false;
}

int main (int argc, char**  argv) {
	load_prc_file_data("", "framebuffer-stencil true");
	load_prc_file("./Config.prc"); // Use a custom .prc file.
	Scene scn (argc, argv, u8"PandaApp");
	scene = &scn;
	//std::setlocale(LC_ALL, "C");
	//std::locale::global(std::locale("C"));
	loadLoc("loc.txt");
	//ImFont* font_body = ImGui::GetIO().Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\Roboto-Regular.ttf", 18.0f, NULL, ImGui::GetIO().Fonts->GetGlyphRangesDefault());
	//IM_ASSERT(body != NULL);
	// Loading models
	//NodePath ground = window->load_model(window->get_render(), "assets/Ground.bam"); ground.set_scale(1, 1, 1);
	//ground.set_name(u8"Ground");
	// Change opacity:
	//ground.set_transparency(TransparencyAttrib::Mode::M_alpha); ground.set_alpha_scale(0.1);
	//ground.set_collide_mask(BitMask32::bit(1));// To make this model collide with pickerRay.
	//scn.toMD(ground);
	// Set tags:
	//ground.set_tag("disp", "y"); // Object will be displayed in the scene graph.
	//ground.clear_tag("op"); // Isn't open

//	CREATE_SCENE

	// Load textures.
	arrtex = TexturePool::load_texture("assets/icons/Arrows.png");
	arrtex->prepare(window->get_graphics_window()->get_gsg()->get_prepared_objects());
	crltex = TexturePool::load_texture("assets/icons/Circles.png");
	crltex->prepare(window->get_graphics_window()->get_gsg()->get_prepared_objects());
	szrtex = TexturePool::load_texture("assets/icons/Sizers.png");
	szrtex->prepare(window->get_graphics_window()->get_gsg()->get_prepared_objects());

    GraphicsWindow* windw = window->get_graphics_window();
    // setup Panda3D mouse for pixel2d
    setup_mouse(window);

    // setup ImGUI for Panda3D
    Panda3DImGui panda3d_imgui_helper(windw, window->get_pixel_2d());
    panda3d_imgui_helper.setup_style(Panda3DImGui::Style::classic);
    panda3d_imgui_helper.setup_geom();
    panda3d_imgui_helper.setup_shader(Filename("shader"));
    panda3d_imgui_helper.setup_font();
    panda3d_imgui_helper.setup_event();
    panda3d_imgui_helper.on_window_resized();
    panda3d_imgui_helper.enable_file_drop();

    // setup Panda3D task and key event
    setup_render(&panda3d_imgui_helper);
    setup_button(window, &panda3d_imgui_helper);

    EventHandler::get_global_event_handler()->add_hook("window-event", [](const Event*, void* user_data) {
        static_cast<Panda3DImGui*>(user_data)->on_window_resized();
    }, &panda3d_imgui_helper);

    // use if the context is in different DLL.
    //ImGui::SetCurrentContext(panda3d_imgui_helper.get_context());

    EventHandler::get_global_event_handler()->add_hook("imgui-new-frame", [](const Event*) {
    	// Call a function to draw my GUI
    	ImGuiNewFrame();
    });

    EventHandler::get_global_event_handler()->add_hook("control-c", &CtrlC, nullptr);
    EventHandler::get_global_event_handler()->add_hook("control-v", &CtrlV, nullptr);
    EventHandler::get_global_event_handler()->add_hook("control-x", &CtrlX, nullptr);

    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetCurrentContext()->PlatformIO.Platform_LocaleDecimalPoint = ImWchar(',');

	frwk.main_loop();
	frwk.close_framework();
	return 0;
}

#define npoint LPoint3f(0.0f, 0.0f, 0.0f)

void loadCodeCpp (std::string& exp, const std::string& npname, const NodePath& np) {
	if(np.has_tag("cpp_code")) {
		exp += u8"#define CURRENT_OBJECT " + npname + u8"\\\n";
		std::ifstream stream(np.get_tag("cpp_code"));
		if (stream.is_open()) {
			std::string line;
			while (std::getline(stream, line)) {
				line += u8" \\\n";
				exp += line;
			}
			stream.close();
		}
		exp += u8"#undef CURRENT_OBJECT\\\n";
	}
}

// Append NodePath to C++ file (used in export).
void appendNP (std::ofstream& stream, const Filename& path, const NodePath& np) {
	if (np.has_tag("exp")) {
		std::string npname = np.get_name();
		NodePath par0 = np.get_parent();
		NodePath par = np.get_parent();
		while (par != scene->mdroot) {
			npname = par.get_name() + "_" + npname;
			par = par.get_parent();
		}
		std::string exp;
		if (np.get_tag("obj") == "m") { // It is a model.
			if (np.has_tag("path")) {
				Filename pth(np.get_tag("path"));
				pth.make_relative_to(path);
				if (par0 == scene->mdroot) {
					exp += u8"NodePath " + npname + u8" = PANDA_WINDOW->load_model(PANDA_ROOT, u8\"" + pth.to_os_specific() + u8"\"); \\\n";
				}
				else {
					exp += u8"NodePath " + npname + u8" = PANDA_WINDOW->load_model(" + npname.substr(0, npname.size() - np.get_name().size() - 1) + u8", u8\"" + pth.to_os_specific() + u8"\"); \\\n";
				}
				if (np.get_pos(scene->mdroot) != npoint) {
					exp += npname + u8".set_pos(PANDA_ROOT, " + fromPoint(np.get_pos(scene->mdroot)) + u8");\\\n";
				}
				if (np.get_hpr(scene->mdroot) != npoint) {
					exp += npname + u8".set_hpr(PANDA_ROOT, " + fromPoint(np.get_hpr(scene->mdroot)) + u8");\\\n";
				}
				if (np.get_scale(scene->mdroot) != npoint) {
					exp += npname + u8".set_scale(PANDA_ROOT, " + fromPoint(np.get_scale(scene->mdroot)) + u8");\\\n";
				}
				exp += npname + u8".set_name(\"" + np.get_name() + u8"\"); \\\n";
				if (np.has_tag("cm")) {
					BitMask32 im = CollisionNode("").get_into_collide_mask();
					std::string tstr1 = "";
					for (short int i = 0; i < 32; i++) {
						if ((np.get_tag("im" + std::to_string(i)) == "t")
								!= im.get_bit(i)) {
							tstr1 += u8"	im.set_bit_to(" + std::to_string(i) + u8", " + ((np.get_tag("im" + std::to_string(i)) == "t") ? std::string(u8"true") : std::string(u8"false")) + u8");\\\n";
						}
					}
					if (tstr1 != "") {
						exp += u8"{\\\n";
						exp += u8"	BitMask32 im = CollisionNode(\"\").get_into_collide_mask();\\\n";
						exp += tstr1;
						exp += u8"	" + npname + u8".set_collide_mask(im);\\\n";
						exp += u8"}\\\n";
					}
				}
			}
		}
		else if (np.get_tag("obj") == "n") { // It is an attached node.
			exp += u8"NodePath " + npname + u8"(\"" + np.get_name() + u8"\"); \\\n";
			if (par0 == scene->mdroot) {
				exp += npname + u8".reparent_to(PANDA_ROOT); \\\n";
			}
			else {
				exp += npname + u8".reparent_to(" + npname.substr(0, npname.size() - np.get_name().size() - 1) + u8");\\\n";
			}
			if (np.get_pos(scene->mdroot) != npoint) {
				exp += npname + u8".set_pos(PANDA_ROOT, " + fromPoint(np.get_pos(scene->mdroot)) + u8");\\\n";
			}
			if (np.get_hpr(scene->mdroot) != npoint) {
				exp += npname + u8".set_hpr(PANDA_ROOT, " + fromPoint(np.get_hpr(scene->mdroot)) + u8");\\\n";
			}
			if (np.get_scale(scene->mdroot) != npoint) {
				exp += npname + u8".set_scale(PANDA_ROOT, " + fromPoint(np.get_scale(scene->mdroot)) + u8");\\\n";
			}
			if (np.has_tag("cm")) {
				BitMask32 im = CollisionNode("").get_into_collide_mask();
				std::string tstr1 = "";
				for (short int i = 0; i < 32; i++) {
					if ((np.get_tag("im" + std::to_string(i)) == "t") != im.get_bit(i)) {
						tstr1 += u8"	im.set_bit_to(" + std::to_string(i) + u8", " + ((np.get_tag("im" + std::to_string(i)) == "t") ? std::string(u8"true") : std::string(u8"false")) + u8");\\\n";
					}
				}
				if (tstr1 != "") {
					exp += u8"{\\\n";
					exp += u8"	BitMask32 im = CollisionNode(\"\").get_into_collide_mask();\\\n";
					exp += tstr1;
					exp += u8"	" + npname + u8".set_collide_mask(im);\\\n";
					exp += u8"}\\\n";
				}
			}
		}
		else if (np.get_tag("obj") == "s") {
			PT(CollisionNode) nd = DCAST(CollisionNode, np.node());
			exp += u8"NodePath " + npname + u8"; \\\n{ \\\n	PT(CollisionNode) node = new CollisionNode (\"" + np.get_name() + u8"\"); \\\n";
			for (unsigned long int i = 0; i < nd->get_num_solids(); i++) {
				exp += u8"	{\\\n";
				if (nd->get_solid(i)->get_type_index() == CollisionSphere::get_class_type().get_index()) {
					const CollisionSphere* sol = static_cast<const CollisionSphere*>(nd->get_solid(i).p());
					exp += u8"	PT(CollisionSphere) solid = new CollisionSphere(" + fromPoint(sol->get_center()) + u8", " + floatWithDot(sol->get_radius()) + u8");\\\n";
				}
				else if (nd->get_solid(i)->get_type_index() == CollisionInvSphere::get_class_type().get_index()) {
					const CollisionInvSphere* sol1 = static_cast<const CollisionInvSphere*> (nd->get_solid(i).p());
					exp += u8"	PT(CollisionInvSphere) solid = new CollisionInvSphere(" + fromPoint(sol1->get_center()) + u8", " + floatWithDot(sol1->get_radius()) +u8");\\\n";
				}
				else if (nd->get_solid(i)->get_type_index() == CollisionCapsule::get_class_type().get_index()) {
					const CollisionCapsule* sol2 = static_cast<const CollisionCapsule*> (nd->get_solid(i).p());
					exp += u8"	PT(CollisionCapsule) solid = new CollisionCapsule(LPoint3f(" + fromPoint(sol2->get_point_a()) + u8"), LPoint3f(" + fromPoint(sol2->get_point_b()) + u8"), " + floatWithDot(sol2->get_radius()) + u8");\\\n";
				}
				else if (nd->get_solid(i)->get_type_index() == CollisionBox::get_class_type().get_index()) {
					const CollisionBox* sol3 = static_cast<const CollisionBox*>(nd->get_solid(i).p());
					LPoint3f dim = sol3->get_dimensions();
					exp += u8"	PT(CollisionBox) solid = new CollisionBox(LPoint3f(" + fromPoint(sol3->get_center()) + "), " + floatWithDot(dim.get_x()) + u8", " + floatWithDot(dim.get_y()) + u8", " + floatWithDot(dim.get_z()) + u8");\\\n";
				}
				else if (nd->get_solid(i)->get_type_index() == CollisionPlane::get_class_type().get_index()) {
					const CollisionPlane* sol4 = static_cast<const CollisionPlane*>(nd->get_solid(i).p());
					LPlanef plane(sol4->get_plane());
					exp += u8"	PT(CollisionPlane) solid = new CollisionPlane(LPlanef(LVector3f(" + fromPoint(plane.get_normal()) + u8"), LPoint3f(" + fromPoint(plane.get_point()) + u8")));\\\n";
				}
				else if (nd->get_solid(i)->get_type_index() == CollisionPolygon::get_class_type().get_index()) {
					const CollisionPolygon* sol5 = static_cast<const CollisionPolygon*>(nd->get_solid(i).p());
					exp += u8"	PT(CollisionPolygon) solid = new CollisionPolygon(LPoint3f(" + fromPoint(sol5->get_point(0)) + u8"), LPoint3f(" + fromPoint(sol5->get_point(1)) + u8"), LPoint3f(" + fromPoint(sol5->get_point(2)) + u8"));\\\n";
				}
				exp += u8"	node->add_solid(solid);\\\n";
				exp += u8"	}\\\n";
			}
			BitMask32 im = CollisionNode("").get_into_collide_mask();
			BitMask32 fm = CollisionNode("").get_from_collide_mask();
			std::string tstr1 = "";
			std::string tstr2 = "";
			for (short int i = 0; i < 32; i++) {
				if ((np.get_tag("im" + std::to_string(i)) == "t") != im.get_bit(i)) {
					tstr1 += u8"	im.set_bit_to(" + std::to_string(i) + u8", " + ((np.get_tag("im" + std::to_string(i)) == "t") ? std::string(u8"true") : std::string(u8"false")) + u8");\\\n";
				}
				if ((np.get_tag("fm" + std::to_string(i)) == "t") != fm.get_bit(i)) {
					tstr2 += u8"	fm.set_bit_to(" + std::to_string(i) + u8", " + ((np.get_tag("fm" + std::to_string(i)) == "t") ? std::string(u8"true") : std::string(u8"false")) + u8");\\\n";
				}
			}
			if (tstr1 != "") {
				exp += u8"	BitMask32 im = CollisionNode(\"\").get_into_collide_mask();\\\n";
				exp += tstr1;
				exp += u8"	node->set_into_collide_mask(im);\\\n";
			}
			if (tstr2 != "") {
				exp += u8"	BitMask32 fm = CollisionNode(\"\").get_from_collide_mask();\\\n";
				exp += tstr2;
				exp += u8"	node->set_from_collide_mask(fm);\\\n";
			}
			if (npname.size() > np.get_name().size()) {
				exp += u8"	" + npname +  u8" = " + npname.substr(0, npname.size() - np.get_name().size() - 1) + u8".attach_new_node(node);\\\n";
			}
			else {
				exp += u8"	" + npname +  u8" = PANDA_ROOT.attach_new_node(node);\\\n";
			}
			if (np.get_pos(scene->mdroot) != npoint) {
				exp += u8"	" + npname + u8".set_pos(PANDA_ROOT, " + fromPoint(np.get_pos(scene->mdroot)) + u8");\\\n";
			}
			if (np.get_hpr(scene->mdroot) != npoint) {
				exp += u8"	" + npname + u8".set_hpr(PANDA_ROOT, " + fromPoint(np.get_hpr(scene->mdroot)) + u8");\\\n";
			}
			if (np.get_scale(scene->mdroot) != npoint) {
				exp += u8"	" + npname + u8".set_scale(PANDA_ROOT, " + fromPoint(np.get_scale(scene->mdroot)) + u8");\\\n";
			}
			exp += u8"}\\\n";
		}
		loadCodeCpp (exp, npname, np);
		stream << exp;

		for (unsigned int i = 0; i < np.get_num_children(); i++) {
			appendNP(stream, path, np.get_child(i));
		}
	}

}
void loadCodePy (std::string& exp, const std::string& npname, const NodePath& np) {
	if(np.has_tag("python_code")) {
		exp += u8"CURRENT_OBLECT = " + npname + u8"\n";
		std::ifstream stream(np.get_tag("python_code"));
		if (stream.is_open()) {
			std::string line;
			while (std::getline(stream, line)) {
				line = u8"	" + line + u8"\n";
				exp += line;
			}
			stream.close();
		}
	}
}
// Append NodePath to Python file (used in export).
void appendNPPy (std::ofstream& stream, const Filename& path, const NodePath& np) {
	if (np.has_tag("exp")) {
		std::string npname = np.get_name();
		NodePath par0 = np.get_parent();
		NodePath par = np.get_parent();
		while (par != scene->mdroot) {
			npname = par.get_name() + "_" + npname;
			par = par.get_parent();
		}
		std::string exp;
		if (np.get_tag("obj") == "m") { // It is a model.
			if (np.has_tag("path")) {
				Filename pth(np.get_tag("path"));
				pth.make_relative_to(path);
				if (par0 == scene->mdroot) {
					exp += u8"	" + npname + u8" = PANDA_LOADER.loadModel(\"" + pth.to_os_specific() + u8"\")\n";
					exp += u8"	" + npname + u8".reparentTo(PANDA_ROOT)\n";
				}
				else {
					exp += u8"	" + npname + u8" = PANDA_LOADER.loadModel(\"" + pth.to_os_specific() + u8"\")\n";
					exp += u8"	" + npname + u8".reparentTo(" + npname.substr(0, npname.size() - np.get_name().size() - 1)  + ")\n";
				}
				if (np.get_pos(scene->mdroot) != npoint) {
					exp += u8"	" + npname + u8".setPos(PANDA_ROOT, " + fromPoint3(np.get_pos(scene->mdroot)) + u8")\n";
				}
				if (np.get_hpr(scene->mdroot) != npoint) {
					exp += u8"	" + npname + u8".setHpr(PANDA_ROOT, " + fromPoint3(np.get_hpr(scene->mdroot)) + u8")\n";
				}
				if (np.get_scale(scene->mdroot) != npoint) {
					exp += u8"	" + npname + u8".setScale(PANDA_ROOT, " + fromPoint3(np.get_scale(scene->mdroot)) + u8")\n";
				}
				exp += u8"	" + npname + u8".setName(\"" + np.get_name() + u8"\")\n";
				if (np.has_tag("cm")) {
					BitMask32 im = CollisionNode("").get_into_collide_mask();
					std::string tstr1 = "";
					for (short int i = 0; i < 32; i++) {
						if ((np.get_tag("im" + std::to_string(i)) == "t")
								!= im.get_bit(i)) {
							tstr1 += u8"	im.setBitTo(" + std::to_string(i) + u8", " + ((np.get_tag("im" + std::to_string(i)) == "t") ? std::string(u8"true") : std::string(u8"false")) + u8")\n";
						}
					}
					if (tstr1 != "") {
						exp += u8"	im = CollisionNode(\"\").getIntoCollide_mask()\n";
						exp += tstr1;
						exp += u8"	" + npname + u8".setCollideMask(im)\n";
					}
				}
			}
		}
		else if (np.get_tag("obj") == "n") { // It is an attached node.
			exp += u8"	" + npname + u8" = NodePath(\"" + np.get_name() + u8"\")\n";
			if (par0 == scene->mdroot) {
				exp += u8"	" + npname + u8".reparentTo(PANDA_ROOT); \\\n";
			}
			else {
				exp += u8"	" +  npname + u8".reparentTo(" + npname.substr(0, npname.size() - np.get_name().size() - 1) + u8")\n";
			}
			if (np.get_pos(scene->mdroot) != npoint) {
				exp += u8"	" +  npname + u8".setPos(PANDA_ROOT, " + fromPoint3(np.get_pos(scene->mdroot)) + u8")\n";
			}
			if (np.get_hpr(scene->mdroot) != npoint) {
				exp += u8"	" +  npname + u8".setHpr(PANDA_ROOT, " + fromPoint3(np.get_hpr(scene->mdroot)) + u8")\n";
			}
			if (np.get_scale(scene->mdroot) != npoint) {
				exp += u8"	" +  npname + u8".setScale(PANDA_ROOT, " + fromPoint3(np.get_scale(scene->mdroot)) + u8")\n";
			}
			if (np.has_tag("cm")) {
				BitMask32 im = CollisionNode("").get_into_collide_mask();
				std::string tstr1 = "";
				for (short int i = 0; i < 32; i++) {
					if ((np.get_tag("im" + std::to_string(i)) == "t") != im.get_bit(i)) {
						tstr1 += u8"	im.setBitTo(" + std::to_string(i) + u8", " + ((np.get_tag("im" + std::to_string(i)) == "t") ? std::string(u8"true") : std::string(u8"false")) + u8");\\\n";
					}
				}
				if (tstr1 != "") {
					exp += u8"	im = CollisionNode(\"\").getIntoCollide_mask()\n";
					exp += tstr1;
					exp += u8"	" + npname + u8".setCollideMask(im);\\\n";
				}
			}
		}
		else if (np.get_tag("obj") == "s") {
			PT(CollisionNode) nd = DCAST(CollisionNode, np.node());
			exp += u8"	" + npname + u8" = NodePath(\"\")\n	node = CollisionNode (\"" + np.get_name() + u8"\")\n";
			for (unsigned long int i = 0; i < nd->get_num_solids(); i++) {
				if (nd->get_solid(i)->get_type_index() == CollisionSphere::get_class_type().get_index()) {
					const CollisionSphere* sol = static_cast<const CollisionSphere*>(nd->get_solid(i).p());
					exp += u8"	solid = CollisionSphere(" + fromPoint3(sol->get_center()) + u8", " + floatWithDot2(sol->get_radius()) + u8")\n";
				}
				else if (nd->get_solid(i)->get_type_index() == CollisionInvSphere::get_class_type().get_index()) {
					const CollisionInvSphere* sol1 = static_cast<const CollisionInvSphere*> (nd->get_solid(i).p());
					exp += u8"	solid = CollisionInvSphere(" + fromPoint3(sol1->get_center()) + u8", " + floatWithDot2(sol1->get_radius()) +u8")\n";
				}
				else if (nd->get_solid(i)->get_type_index() == CollisionCapsule::get_class_type().get_index()) {
					const CollisionCapsule* sol2 = static_cast<const CollisionCapsule*> (nd->get_solid(i).p());
					exp += u8"	solid = CollisionCapsule(LPoint3f(" + fromPoint3(sol2->get_point_a()) + u8"), LPoint3f(" + fromPoint3(sol2->get_point_b()) + u8"), " + floatWithDot2(sol2->get_radius()) + u8")\n";
				}
				else if (nd->get_solid(i)->get_type_index() == CollisionBox::get_class_type().get_index()) {
					const CollisionBox* sol3 = static_cast<const CollisionBox*>(nd->get_solid(i).p());
					LPoint3f dim = sol3->get_dimensions();
					exp += u8"	solid = CollisionBox(LPoint3f(" + fromPoint3(sol3->get_center()) + "), " + floatWithDot2(dim.get_x()) + u8", " + floatWithDot2(dim.get_y()) + u8", " + floatWithDot2(dim.get_z()) + ")\n";
				}
				else if (nd->get_solid(i)->get_type_index() == CollisionPlane::get_class_type().get_index()) {
					const CollisionPlane* sol4 = static_cast<const CollisionPlane*>(nd->get_solid(i).p());
					LPlanef plane(sol4->get_plane());
					exp += u8"	solid = CollisionPlane(LPlanef(LVector3f(" + fromPoint3(plane.get_normal()) + u8"), LPoint3f(" + fromPoint3(plane.get_point()) + u8")))\n";
				}
				else if (nd->get_solid(i)->get_type_index() == CollisionPolygon::get_class_type().get_index()) {
					const CollisionPolygon* sol5 = static_cast<const CollisionPolygon*>(nd->get_solid(i).p());
					exp += u8"	solid = CollisionPolygon(LPoint3f(" + fromPoint3(sol5->get_point(0)) + u8"), LPoint3f(" + fromPoint3(sol5->get_point(1)) + u8"), LPoint3f(" + fromPoint3(sol5->get_point(2)) + u8"))\n";
				}
				exp += u8"	node.addSolid(solid)\n";
			}
			BitMask32 im = CollisionNode("").get_into_collide_mask();
			BitMask32 fm = CollisionNode("").get_from_collide_mask();
			std::string tstr1 = "";
			std::string tstr2 = "";
			for (short int i = 0; i < 32; i++) {
				if ((np.get_tag("im" + std::to_string(i)) == "t") != im.get_bit(i)) {
					tstr1 += u8"	im.setBitTo(" + std::to_string(i) + u8", " + ((np.get_tag("im" + std::to_string(i)) == "t") ? std::string(u8"true") : std::string(u8"false")) + u8")\n";
				}
				if ((np.get_tag("fm" + std::to_string(i)) == "t") != fm.get_bit(i)) {
					tstr2 += u8"	fm.setBitTo(" + std::to_string(i) + u8", " + ((np.get_tag("fm" + std::to_string(i)) == "t") ? std::string(u8"true") : std::string(u8"false")) + u8")\n";
				}
			}
			if (tstr1 != "") {
				exp += u8"	im = CollisionNode(\"\").getIntoCollideMask()\n";
				exp += tstr1;
				exp += u8"	node.setIntoCollideMask(im)\n";
			}
			if (tstr2 != "") {
				exp += u8"	fm = CollisionNode(\"\").getFromCollideMask()\n";
				exp += tstr2;
				exp += u8"	node.setFromCollideMask(fm)\n";
			}
			if (npname.size() > np.get_name().size()) {
				exp += u8"	" + npname +  u8" = " + npname.substr(0, npname.size() - np.get_name().size() - 1) + u8".attachNewNode(node)\n";
			}
			else {
				exp += u8"	" + npname +  u8" = PANDA_ROOT.attachNewNode(node)\n";
			}
			if (np.get_pos(scene->mdroot) != npoint) {
				exp += u8"	" + npname + u8".setPos(PANDA_ROOT, " + fromPoint3(np.get_pos(scene->mdroot)) + u8")\n";
			}
			if (np.get_hpr(scene->mdroot) != npoint) {
				exp += u8"	" + npname + u8".setHpr(PANDA_ROOT, " + fromPoint3(np.get_hpr(scene->mdroot)) + u8")\n";
			}
			if (np.get_scale(scene->mdroot) != npoint) {
				exp += u8"	" + npname + u8".setScale(PANDA_ROOT, " + fromPoint3(np.get_scale(scene->mdroot)) + u8")\n";
			}
			if(np.has_tag("python_code")) {
				exp += u8"CURRENT_OBLECT = " + npname + u8"\n";
				std::ifstream stream(np.get_tag("python_code"));
				if (stream.is_open()) {
					std::string line;
					while (std::getline(stream, line)) {
						line = u8"	" + line + u8"\n";
						exp += line;
					}
					stream.close();
				}
			}
		}
		loadCodePy(exp,npname, np);
		stream << exp;
		for (unsigned int i = 0; i < np.get_num_children(); i++) {
			appendNPPy(stream, path, np.get_child(i));
		}
	}

}
// Gets Collision Mask data from NodePath tags to string.
std::string fromCM (const NodePath& np, const std::string& mask = u8"im") {
	std::string res = "";
	if (np.has_tag(mask + u8"0")) {
		for (short int i = 0; i < 32; i++) {
			res += np.get_tag(mask + std::to_string(i));
		}
	}
	else {
		NodePath np("");
		setMaskTagsIm(np);
		for (short int i = 0; i < 32; i++) {
			res += np.get_tag(mask + std::to_string(i));
		}
	}
	return res;
}
// Pushes Collision Mask data to NodePath tags from string.
void toCM (NodePath& np, const std::string& mask, const std::string& mtype = "im") {
	for (short int i = 0; i < mask.size(); i++) {
		np.set_tag(mtype + std::to_string(i), {mask[i]} );
	}
}

// Look above.
void loadNP(std::ifstream& stream, const NodePath& parent) {
	std::string line;
	PT(CollisionNode) nd;
	NodePath np;
	char type = ' ';
	std::getline(stream, line);
	{
		std::vector<std::string> vec;
		parseLine(vec, line);
		line = vec[1];
		if (vec[0] == u8"n") { // It is a node.
			PT(ModelRoot) mr = new ModelRoot(line);
			np = parent.attach_new_node(mr);
			np.set_name(getCorrName(np));
			type = 'n';
			np.set_tag("obj", "n");
			np.set_tag("disp", "y");
			std::getline(stream, line);// It is {
		}
		else if (vec[0] == u8"m") { // It is a model.
			std::string name = line;
			type = 'm';
			std::getline(stream, line); // It is {
			std::iostream::pos_type pos = stream.tellg();
			std::getline(stream, line);
			std::vector<std::string> vc;
			parseLine(vc, line);
			if (vc[0] == u8"path") {
				np = window->load_model(parent, vc[1]);
				np.set_name(name);
				np.set_name(getCorrName(np));
				np.set_tag("obj", "m");
				np.set_tag("disp", "y");
				np.set_tag("path", vc[1]);
			}
			else {
				stream.seekg(pos);
				PT(ModelRoot) mr = new ModelRoot(name);
				np = parent.attach_new_node(mr);
				np.set_name(getCorrName(np));
				np.reparent_to(parent);
				np.set_tag("obj", "m");
				np.set_tag("disp", "y");
			}
			np.set_collide_mask(BitMask32::bit(1));
		}
		else if (vec[0] == u8"s") { // It is a collision node.
			nd = new CollisionNode(line);
			np = parent.attach_new_node(nd);
			np.set_name(getCorrName(np));
			type = 's';
			np.set_tag("obj", "s");
			np.set_tag("disp", "y");
			std::getline(stream, line);// It is {
		}
	}
	bool stop = false;
	while (!stop && std::getline(stream, line)) {
		std::vector<std::string> vec;
		parseLine(vec, line);
		line = vec[0];
		if (line == u8"pos") {
			LPoint3f pos(0.0f, 0.0f, 0.0f);
			for (short int i = 0; i < 3 && i < vec.size() - 1; i++) {
				fast_float::from_chars(vec[i+1].data(), vec[i+1].data() + vec[i+1].size(), pos[i]);
				//pos[i] = std::stof(vec[i + 1]);
			}
			np.set_pos(pos);
		}
		else if (line == u8"hpr") {
			LPoint3f hpr(0.0f, 0.0f, 0.0f);
			for (short int i = 0; i < 3 && i < vec.size() - 1; i++) {
				fast_float::from_chars(vec[i+1].data(), vec[i+1].data() + vec[i+1].size(), hpr[i]);
				//hpr[i] = std::stof(vec[i + 1]);
			}
			np.set_hpr(hpr);
		}
		else if (line == u8"scale") {
			LPoint3f scale(0.0f, 0.0f, 0.0f);
			for (short int i = 0; i < 3 && i < vec.size() - 1; i++) {
				fast_float::from_chars(vec[i+1].data(), vec[i+1].data() + vec[i+1].size(), scale[i]);
				//scale[i] = std::stof(vec[i + 1]);
			}
			np.set_scale(scale);
		}
		else if (line == u8"export") {
			if (vec.size() > 1) {
				line = vec[1];
				if (line == u8"t" || line == u8"t ") {
					np.set_tag("exp", "y");
				}
			}
		}
		else if (line == u8"path") {
			if (vec.size() > 1 && type == 'm') {
				line = vec[1];
				NodePath mod = window->load_model(parent, line);
				mod.set_tag("path", line);
				mod.set_collide_mask(BitMask32::bit(1));
				if (np.has_tag("exp")) {
					mod.set_tag("exp", np.get_tag("exp"));
				}
				mod.set_tag("obj", np.get_tag("obj"));
				mod.set_name(np.get_name());
				mod.set_tag("disp", "y");
				if (np.has_tag("cpp_code")) {
					mod.set_tag("cpp_code", np.get_tag("cpp_code"));
				}
				if (np.has_tag("python_code")) {
					mod.set_tag("python_code", np.get_tag("python_code"));
				}
				if (np.has_tag("cm")) {
					mod.set_tag("cm", "y");
					toCM(mod, fromCM(np, "im"), "im");
				}
				mod.set_pos(np.get_pos());
				mod.set_hpr(np.get_hpr());
				mod.set_scale(np.get_scale());
				np.remove_node();
				np = mod;
			}
		}
		else if (line == u8"cmask") {
			if (vec.size() > 1) {
				line = vec[1];
				toCM(np, line, "im");
				np.set_tag("cm", "y");
			}
		}
		else if (line == u8"imask") {
			if (vec.size() > 1) {
				line = vec[1];
				toCM(np, line, "im");
			}
		}
		else if (line == u8"fmask") {
			if (vec.size() > 1) {
				line = vec[1];
				toCM(np, line, "fm");
			}
		}
		else if (line == u8"show") {
			if (vec.size() > 1) {
				line = vec[1];
				if (line == u8"t" || line == u8"t ") {
					np.show();
					np.set_tag("shs", "y");
					nd->set_into_collide_mask(BitMask32::bit(1));
				}
				else {
					np.hide();
					np.clear_tag("shs");
					nd->set_into_collide_mask(BitMask32::all_off());
				}
			}
		}
		else if (line == u8"sphere") {
			if (!nd.is_null()) {
				LPoint3f cent(0.0f, 0.0f, 0.0f);
				float rad = 0.0f;
				for (short int i = 0; i < 3 && i < vec.size() - 1; i++) {
					fast_float::from_chars(vec[i+1].data(), vec[i+1].data() + vec[i+1].size(), cent[i]);
					//cent[i] = std::stof(vec[i + 1]);
				}
				if (vec.size() > 4) {
					fast_float::from_chars(vec[4].data(), vec[4].data() + vec[4].size(), rad);
					//rad = std::stof(vec[4]);
				}
				PT(CollisionSphere) solid = new CollisionSphere(cent, rad);
				nd->add_solid(solid);
			}
		}
		else if (line == u8"invsphere") {
			if (!nd.is_null()) {
				LPoint3f cent(0.0f, 0.0f, 0.0f);
				float rad = 0.0f;
				for (short int i = 0; i < 3 && i < vec.size() - 1; i++) {
					fast_float::from_chars(vec[i+1].data(), vec[i+1].data() + vec[i+1].size(), cent[i]);
					//cent[i] = std::stof(vec[i + 1]);
				}
				if (vec.size() > 4) {
					fast_float::from_chars(vec[4].data(), vec[4].data() + vec[4].size(), rad);
					//rad = std::stof(vec[4]);
				}
				PT(CollisionInvSphere) solid = new CollisionInvSphere(cent,rad);
				nd->add_solid(solid);
			}
		}
		else if (line == u8"capsule") {
			if (!nd.is_null()) {
				LPoint3f p1(0.0f, 0.0f, 0.0f);
				LPoint3f p2(0.0f, 0.0f, 0.0f);
				float rad = 0.0f;
				for (short int i = 0; i < 3 && i < vec.size() - 1; i++) {
					fast_float::from_chars(vec[i+1].data(), vec[i+1].data() + vec[i+1].size(), p1[i]);
					//p1[i] = std::stof(vec[i + 1]);
				}
				for (short int i = 0; i < 3 && i < vec.size() - 4; i++) {
					fast_float::from_chars(vec[i+4].data(), vec[i+4].data() + vec[i+4].size(), p2[i]);
					//p2[i] = std::stof(vec[i + 4]);
				}
				if (vec.size() > 7) {
					fast_float::from_chars(vec[7].data(), vec[7].data() + vec[7].size(), rad);
					//rad = std::stof(vec[7]);
				}
				PT(CollisionCapsule) solid = new CollisionCapsule(p1, p2, rad);
				nd->add_solid(solid);
			}
		}
		else if (line == u8"box") {
			if (!nd.is_null()) {
				LPoint3f cent(0.0f, 0.0f, 0.0f);
				LPoint3f dim(0.0f, 0.0f, 0.0f);
				for (short int i = 0; i < 3 && i < vec.size() - 1; i++) {
					fast_float::from_chars(vec[i+1].data(), vec[i+1].data() + vec[i+1].size(), cent[i]);
					//cent[i] = std::stof(vec[i + 1]);
				}
				for (short int i = 0; i < 3 && i < vec.size() - 4; i++) {
					fast_float::from_chars(vec[i+4].data(), vec[i+4].data() + vec[i+4].size(), dim[i]);
					//dim[i] = std::stof(vec[i + 4]);
				}
				PT(CollisionBox) solid = new CollisionBox(cent, dim[0], dim[1], dim[2]);
				nd->add_solid(solid);
			}
		}
		else if (line == u8"plane") {
			if (!nd.is_null()) {
				LPoint3f norm(0.0f, 0.0f, 0.0f);
				LPoint3f point(0.0f, 0.0f, 0.0f);
				for (short int i = 0; i < 3 && i < vec.size() - 1; i++) {
					fast_float::from_chars(vec[i+1].data(), vec[i+1].data() + vec[i+1].size(), norm[i]);
					//norm[i] = std::stof(vec[i + 1]);
				}
				for (short int i = 0; i < 3 && i < vec.size() - 4; i++) {
					fast_float::from_chars(vec[i+4].data(), vec[i+4].data() + vec[i+4].size(), point[i]);
					//point[i] = std::stof(vec[i + 4]);
				}
				PT(CollisionPlane) solid = new CollisionPlane(LPlanef(norm, point));
				nd->add_solid(solid);
			}
		}
		else if (line == u8"polygon") {
			if (!nd.is_null()) {
				LPoint3f p1(0.0f, 0.0f, 0.0f);
				LPoint3f p2(0.0f, 0.0f, 0.0f);
				LPoint3f p3(0.0f, 0.0f, 0.0f);
				for (short int i = 0; i < 3 && i < vec.size() - 1; i++) {
					fast_float::from_chars(vec[i+1].data(), vec[i+1].data() + vec[i+1].size(), p1[i]);
					//p1[i] = std::stof(vec[i + 1]);
				}
				for (short int i = 0; i < 3 && i < vec.size() - 4; i++) {
					fast_float::from_chars(vec[i+4].data(), vec[i+4].data() + vec[i+4].size(), p2[i]);
					//p2[i] = std::stof(vec[i + 4]);
				}
				for (short int i = 0; i < 3 && i < vec.size() - 7; i++) {
					fast_float::from_chars(vec[i+7].data(), vec[i+7].data() + vec[i+7].size(), p3[i]);
					//p3[i] = std::stof(vec[i + 7]);
				}
				PT(CollisionPolygon) solid = new CollisionPolygon(p1, p2, p3);
				nd->add_solid(solid);
			}
		}
		else if (line == u8"cpp_code") {
			if (vec.size() > 1) {
				np.set_tag("cpp_code", vec[1]);
			}
		}
		else if (line == u8"python_code") {
			if (vec.size() > 1) {
				np.set_tag("python_code", vec[1]);
			}
		}
		else if (line == u8"children") {
			while (checkNP(stream)) {
				loadNP(stream, np);
			}
		}
		else if (line == u8"}") {
			stop = true;
		}
	}
}
// Look above.
void saveNP(std::ofstream& stream, const NodePath& np, const std::string& tabs) {
	if (np == scene->mdroot) {
		for (long int i = 0; i < np.get_num_children(); i++) {
			saveNP(stream, np.get_child(i), tabs);
		}
		return;
	}
	bool chSaved = false;
	std::string exp;
	if (np.get_tag("obj") == "m") { // It is a model.
			if (np.has_tag("path")) {
				exp += tabs + u8"m " + np.get_name() + u8"\n";
				exp += tabs + u8"{\n";
				exp += tabs + u8"path " + np.get_tag("path") + u8"\n";
				exp += tabs + u8"pos " + fromPoint2(np.get_pos()) + u8" \n";
				exp += tabs + u8"hpr " + fromPoint2(np.get_hpr()) + u8" \n";
				exp += tabs + u8"scale " + fromPoint2(np.get_scale()) + u8" \n";
				exp += tabs + u8"export " + (np.has_tag("exp") ? u8"t" : u8"f") + u8"\n";
				if (np.has_tag("cpp_code")) {
					exp += tabs + u8"cpp_code " + np.get_tag("cpp_code") + u8" \n";
				}
				if (np.has_tag("python_code")) {
					exp += tabs + u8"python_code " + np.get_tag("python_code") + u8" \n";
				}
				if (np.has_tag("cm")) {
					exp += tabs + u8"cmask " + fromCM(np) + u8"\n";
				}
				if (np.get_num_children() != 0) {
					exp += tabs + u8"children \n";
					chSaved = true;
					stream << exp;
					for (long int i = 0; i < np.get_num_children(); i++) {
						saveNP(stream, np.get_child(i), tabs + u8"	");
					}
					stream << tabs + u8"} \n";
				}
				exp += tabs + u8"} \n";
			}
	}
	else if (np.get_tag("obj") == "n") {
		exp += tabs + u8"n " + np.get_name() + u8"\n";
		exp += tabs + u8"{\n";
		exp += tabs + u8"pos " + fromPoint2(np.get_pos()) + u8" \n";
		exp += tabs + u8"hpr " + fromPoint2(np.get_hpr()) + u8" \n";
		exp += tabs + u8"scale " + fromPoint2(np.get_scale()) + u8" \n";
		exp += tabs + u8"export " + (np.has_tag("exp") ? u8"t" : u8"f") + u8"\n";
		if (np.has_tag("cpp_code")) {
			exp += tabs + u8"cpp_code " + np.get_tag("cpp_code");
		}
		if (np.has_tag("python_code")) {
			exp += tabs + u8"python_code " + np.get_tag("python_code");
		}
		if (np.has_tag("cm")) {
			exp += tabs + u8"cmask " + fromCM(np, "im") + u8"\n";
		}
		if (np.get_num_children() != 0) {
			exp += tabs + u8"children \n";
			chSaved = true;
			stream << exp;
			for (long int i = 0; i < np.get_num_children(); i++) {
				saveNP(stream, np.get_child(i), tabs + u8"	");
			}
			stream << tabs + u8"} \n";
		}
		exp += tabs + u8"} \n";
	}
	else if (np.get_tag("obj") == "s") {
		exp += tabs + u8"s " + np.get_name() + u8"\n";
		exp += tabs + u8"{\n";
		exp += tabs + u8"pos " + fromPoint2(np.get_pos()) + u8" \n";
		exp += tabs + u8"hpr " + fromPoint2(np.get_hpr()) + u8" \n";
		exp += tabs + u8"scale " + fromPoint2(np.get_scale()) + u8" \n";
		exp += tabs + u8"export " + (np.has_tag("exp") ? u8"t" : u8"f") + u8"\n";
		exp += tabs + u8"show " + (np.has_tag("shs") ? u8"t" : u8"f") + u8"\n"; // Show solids?
		if (np.has_tag("cpp_code")) {
			exp += tabs + u8"cpp_code " + np.get_tag("cpp_code");
		}
		if (np.has_tag("python_code")) {
			exp += tabs + u8"python_code " + np.get_tag("python_code");
		}
		PT(CollisionNode) nd = DCAST(CollisionNode, np.node());
		for (unsigned long int i = 0; i < nd->get_num_solids(); i++) {
			if (nd->get_solid(i)->get_type_index() == CollisionSphere::get_class_type().get_index()) {
				const CollisionSphere* sol = static_cast<const CollisionSphere*>(nd->get_solid(i).p());
				exp += tabs + u8"sphere " + fromPoint2(sol->get_center()) + u8" " + floatWithDot2(sol->get_radius()) + u8"\n";
			}
			else if (nd->get_solid(i)->get_type_index() == CollisionInvSphere::get_class_type().get_index()) {
				const CollisionInvSphere* sol1 = static_cast<const CollisionInvSphere*> (nd->get_solid(i).p());
				exp += tabs + u8"invsphere " + fromPoint2(sol1->get_center()) + u8" " + floatWithDot2(sol1->get_radius()) +u8"\n";
			}
			else if (nd->get_solid(i)->get_type_index() == CollisionCapsule::get_class_type().get_index()) {
				const CollisionCapsule* sol2 = static_cast<const CollisionCapsule*> (nd->get_solid(i).p());
				exp += tabs + u8"capsule " + fromPoint2(sol2->get_point_a()) + u8" " + fromPoint2(sol2->get_point_b()) + u8" " + floatWithDot2(sol2->get_radius()) + u8"\n";
			}
			else if (nd->get_solid(i)->get_type_index() == CollisionBox::get_class_type().get_index()) {
				const CollisionBox* sol3 = static_cast<const CollisionBox*>(nd->get_solid(i).p());
				LPoint3f dim = sol3->get_dimensions() / 2;
				exp += tabs + u8"box " + fromPoint2(sol3->get_center()) + " " + fromPoint2(dim) + "\n";
			}
			else if (nd->get_solid(i)->get_type_index() == CollisionPlane::get_class_type().get_index()) {
				const CollisionPlane* sol4 = static_cast<const CollisionPlane*>(nd->get_solid(i).p());
				LPlanef plane(sol4->get_plane());
				exp += tabs + u8"plane " + fromPoint2(plane.get_normal()) + u8" " + fromPoint2(plane.get_point()) + u8"\n";
			}
			else if (nd->get_solid(i)->get_type_index() == CollisionPolygon::get_class_type().get_index()) {
				const CollisionPolygon* sol5 = static_cast<const CollisionPolygon*>(nd->get_solid(i).p());
				exp += tabs + u8"polygon " + fromPoint2(sol5->get_point(0)) + u8" " + fromPoint2(sol5->get_point(1)) + u8" " + fromPoint2(sol5->get_point(2)) + u8"\n";
			}
		}
		exp += tabs + u8"imask " + fromCM(np, "im") + u8"\n";
		exp += tabs + u8"fmask " + fromCM(np, "fm") + u8"\n";
		if (np.get_num_children() != 0) {
			exp += tabs + u8"children \n";
			chSaved = true;
			stream << exp;
			for (long int i = 0; i < np.get_num_children(); i++) {
				saveNP(stream, np.get_child(i), tabs + u8"	");
			}
			stream << tabs + u8"} \n";
		}
		exp += tabs + u8"} \n";
	}
	if (!chSaved) {
		stream << exp;
	}
}
// Look above.
std::pair<bool, std::string> ExportCppMacro(const Filename& path) {
	std::ofstream stream (path.get_fullpath());
	if (!stream.is_open()) {
		return std::pair<bool, std::string>(false, "cnt_open_outp");
	}
	stream << "// This header is auto-generated. Use \"CREATE_SCENE\" macro to load the scene. Before including this, you have to define macros PANDA_WINDOW and PANDA_ROOT and set it to your WindowFramework's pointer name and your NodePath root's name.\n#define CREATE_SCENE \\\n";
	for (unsigned long int i = 0; i < scene->mdroot.get_num_children(); i++) {
		appendNP(stream, path, scene->mdroot.get_child(i));
	}
	stream << "\n";
	stream.close();
	return std::pair<bool, std::string>(true, "exp_success");
}
// Look above.
std::pair<bool, std::string> ExportPy(const Filename& path) {
	std::ofstream stream (path.get_fullpath());
	if (!stream.is_open()) {
		return std::pair<bool, std::string>(false, "cnt_open_outp");
	}
	stream << "# This file is auto-generated. Use \"CREATE_SCENE\" to load the scene.\nfrom panda3d.core import NodePath\nfrom panda3d.core import CollisionNode\nfrom panda3d.core import LPoint3f\nfrom panda3d.core import CollideMask\nfrom panda3d.core import CollisionSphere\nfrom panda3d.core import CollisionInvSphere\nfrom panda3d.core import CollisionPlane\nfrom panda3d.core import CollisionCapsule\nfrom panda3d.core import CollisionBox\nfrom panda3d.core import CollisionPolygon\ndef CREATE_SCENE(PANDA_ROOT, PANDA_LOADER):\n";
	for (unsigned long int i = 0; i < scene->mdroot.get_num_children(); i++) {
		appendNPPy(stream, path, scene->mdroot.get_child(i));
	}
	stream << "\n";
	stream.close();
	return std::pair<bool, std::string>(true, "exp_success");
}
