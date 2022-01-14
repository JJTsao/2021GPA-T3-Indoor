#version 420

layout(location = 0) in vec3 iv3vertex;
layout(location = 1) in vec2 iv2tex_coord;
layout(location = 2) in vec3 iv3normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

uniform mat4 m_mat;
uniform mat4 v_mat;
uniform mat4 p_mat;

void main()                                                                                                                                          
{                                                                                                                                                                                                                                                       
    gl_Position = p_mat * v_mat * m_mat * vec4(iv3vertex, 1.0);
}