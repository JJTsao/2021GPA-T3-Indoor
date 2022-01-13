#version 420

layout(location = 0) in vec3 iv3vertex;
layout(location = 1) in vec2 iv2tex_coord;
layout(location = 2) in vec3 iv3normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

uniform mat4 m_mat;
uniform mat4 v_mat;
uniform mat4 p_mat;

out VertexData
{
	vec3 v_normal;
	vec3 u_normal;
	vec3 view;
} vertexData;

void main()                                                                                                                                          
{                                                                                                                                                    
	vec4 pos_vs = v_mat * m_mat * vec4(iv3vertex, 1.0);
    vertexData.v_normal = mat3(v_mat * m_mat) * iv3normal;
	vertexData.u_normal = mat3(m_mat) * iv3normal;
    vertexData.view = pos_vs.xyz;                                                                                                                
    gl_Position = p_mat * pos_vs;
}