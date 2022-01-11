#version 420

layout(location = 0) in vec3 iv3vertex;
layout(location = 1) in vec2 iv2tex_coord;
layout(location = 2) in vec3 iv3normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

uniform mat4 m_mat;
uniform mat4 v_mat;
uniform mat4 p_mat;
uniform mat4 shadow_matrix;

vec3 directional_light_pos = vec3(-2.51449f, 0.477241f, -1.21263f);

out VertexData
{
	vec3 normal;
    vec2 texcoord;
	vec3 N;
	vec3 L;
	vec3 V;
	vec3 eyeDir;
	vec3 lightDir;
	vec4 shadow_coord;
} vertexData;

void main()
{
	vec4 pos_vs = m_mat * vec4(iv3vertex, 1.0);

	vec3 T = normalize(mat3(m_mat) * tangent);
	vec3 N = normalize(mat3(m_mat) * iv3normal);
	vec3 B = normalize(mat3(m_mat) * bitangent);
	vec3 L = directional_light_pos - pos_vs.xyz;
	vec3 V = -pos_vs.xyz;
	
	vertexData.normal = iv3normal; // no use
    vertexData.texcoord = iv2tex_coord;

	vertexData.N = mat3(m_mat) * iv3normal;
	vertexData.L = L;
	vertexData.V = V;

	vertexData.eyeDir = normalize( vec3( dot(V, T), dot(V, B), dot(V, N) ) );
	vertexData.lightDir = normalize( vec3( dot(L, T), dot(L, B), dot(L, N) ) );

	vertexData.shadow_coord = shadow_matrix * vec4(iv3vertex, 1.0);

	gl_Position = p_mat * v_mat * pos_vs;
}