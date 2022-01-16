#version 410

layout (location = 0) in vec3 iv3vertex;
layout (location = 1) in vec2 iv2tex_coord;
layout (location = 2) in vec3 iv3normal;

uniform mat4 m_mat;
uniform mat4 vp_mat;

out VS_OUT
{
	vec3 position;
	vec2 texcoord;
	vec3 normal;
} vs_out;

void main()
{
	gl_Position = vp_mat * m_mat * vec4(iv3vertex, 1.0);
	vs_out.position = mat3(m_mat) * iv3vertex;
	vs_out.texcoord = iv2tex_coord;
	vs_out.normal = mat3(m_mat) * iv3normal;
}