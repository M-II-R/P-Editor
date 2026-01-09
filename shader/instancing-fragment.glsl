//GLSL 
#version 450

uniform sampler2D p3d_Texture0; 
uniform mat4 p3d_ModelMatrix;
uniform mat4 p3d_ViewMatrix;
uniform struct p3d_FogParameters {
  vec4 color;
  float density;
} p3d_Fog;
layout (binding = 0, std430) buffer shader_data_buf {
  vec4 shader_data[];
};
in vec4 vTexCoord;
in vec4 ver_pos;
flat in int inst_ID;

out vec4 FragColor;

void main() {
  vec4 model_pos = p3d_ModelMatrix * shader_data[ inst_ID ];//vec4(0, 0, 0, 1);
  vec4 model_pos_view = p3d_ViewMatrix * model_pos;
  FragColor = texture(p3d_Texture0, vTexCoord.st);
  FragColor.rgb = mix(p3d_Fog.color.rgb, FragColor.rgb, clamp(exp2(p3d_Fog.density * gl_FragCoord.z * -length(model_pos_view.xyz)), 0.0f, 1.0f));
}
