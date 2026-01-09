#include "grid.hpp"

LPoint3f roundPos (const LPoint3f& pos, const int roundx, const int roundy, const int roundz) {
	LPoint3f res = pos;
	res.set_x(((int)pos.get_x())/roundx * roundx - 2.5f); // To make grid be placed better.
	res.set_y(((int)pos.get_y())/roundy * roundy);
	res.set_z(((int)pos.get_z())/roundz * roundz);
	return res;
}
	Grid::Grid () {}
	void Grid::Init (const NodePath& parent, WindowFramework* window, const int count) { // Initialize grid. Must be called before use, only once.
		grid = window->load_model(parent, "assets/Grid.bam");
		grid.node()->set_bounds(new OmniBoundingVolume());
		grid.set_instance_count(count * count);
		PT(Shader) shader = Shader::load(Shader::SL_GLSL, "shader/instancing-vertex.glsl", "shader/instancing-fragment.glsl");
		PTA(UnalignedLVecBase4f) sdata (count * count, *(new UnalignedLVecBase4f[count * count]));
		for (int i = 0; i < sdata.size(); i++) {
			sdata[i] = UnalignedLVecBase4f(-40.0f * (count/2) + 40.0f * (i%count), 0.0f, -2.0f * (count/2) + 2.0f * (i/count), 0.0f);
		}
		vector_uchar bdata;
		bdata.resize(sizeof(UnalignedLVecBase4f) * sdata.size());
		const unsigned char* data = reinterpret_cast<const unsigned char*>(sdata.p());
		std::copy(data, data + sizeof(UnalignedLVecBase4f) * sdata.size(), bdata.begin());
		PT(ShaderBuffer) buf = new ShaderBuffer ("shader_data_buf", bdata, GeomEnums::UsageHint::UH_static);
		grid.set_shader(shader);
		grid.set_shader_input("shader_data_buf", buf);
	}
	void Grid::place (const LPoint3f& center, const int round, const float scale) { // Place grid on the scene correctly (grid cells look like they stay in the same position).
		grid.set_pos(roundPos(center, round, round, round));
		grid.set_scale(scale);
	}
