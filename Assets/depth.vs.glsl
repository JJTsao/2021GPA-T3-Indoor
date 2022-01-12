#version 410

uniform mat4 m_mat;
uniform mat4 v_mat;
uniform mat4 p_mat;

layout (location = 0) in vec3 position;

void main()
{
	gl_Position = p_mat * v_mat * m_mat * vec4(position, 1.0);
}