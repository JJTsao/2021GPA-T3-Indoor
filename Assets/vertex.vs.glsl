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
vec3 point_light_pos = vec3(1.87659f, 0.4625f, 0.103928f);

out vec3 eyeDir;
out vec3 t_eyeDir;

out vec3 dirLightDir;
out vec3 t_dirLightDir;
out vec3 pointLightDir;
out vec3 t_pointLightDir;
out vec4 shadow_coord;

out VertexData
{
	vec2 texcoord;
	vec3 normal;
} vertexData;

void main()
{
	vec4 pos_vs = m_mat * vec4(iv3vertex, 1.0);

	vec3 T = normalize(mat3(m_mat) * tangent);
	vec3 N = normalize(mat3(m_mat) * iv3normal);
	vec3 B = normalize(mat3(m_mat) * bitangent);

	eyeDir = -pos_vs.xyz;
	t_eyeDir = normalize( vec3( dot(eyeDir, T), dot(eyeDir, B), dot(eyeDir, N) ) );

	dirLightDir = directional_light_pos - pos_vs.xyz;
	t_dirLightDir = normalize( vec3( dot(dirLightDir, T), dot(dirLightDir, B), dot(dirLightDir, N) ) );
	pointLightDir = point_light_pos - pos_vs.xyz;
	t_pointLightDir = normalize( vec3( dot(pointLightDir, T), dot(pointLightDir, B), dot(pointLightDir, N) ) );
	shadow_coord = shadow_matrix * vec4(iv3vertex, 1.0);

	vertexData.texcoord = iv2tex_coord;
	vertexData.normal = N;

	gl_Position = p_mat * v_mat * pos_vs;
}