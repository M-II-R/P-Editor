//GLSL 
#version 450
//#extension GL_ARB_compatibility : enable

uniform mat4 p3d_ModelViewProjectionMatrix; 
in vec4 p3d_Vertex; 
in vec4 p3d_MultiTexCoord0; 
layout (binding = 0, std430) buffer shader_data_buf {
  vec4 shader_data[];
};
out vec4 vTexCoord;
out vec4 ver_pos;
out int inst_ID;

void main() {
  gl_Position = p3d_ModelViewProjectionMatrix * (p3d_Vertex + shader_data[ gl_InstanceID ] );
  vTexCoord = p3d_MultiTexCoord0; 
  inst_ID = gl_InstanceID;
}
