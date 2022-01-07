#version 420

uniform sampler2D tex;
uniform sampler2D trice_normal;
uniform int default_tex;
uniform int obj_id;
uniform int normal_mapping_flag;
uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;

in VertexData
{
	vec3 normal;
    vec2 texcoord;
	vec3 N;
	vec3 L;
	vec3 V;
	vec3 eyeDir;
	vec3 lightDir;
} vertexData;

out vec4 fragColor;

float specular_power = 128.0;
vec3 Ia = vec3(0.45);
vec3 Id = vec3(0.45);
vec3 Is = vec3(0.1);

void main()
{
	// pure phong shading
	vec3 N = normalize(vertexData.N);
	vec3 L = normalize(vertexData.L);
	vec3 V = normalize(vertexData.V);

	vec3 R = reflect(-L, N);

	vec3 tex_diffuse = texture(tex, vertexData.texcoord).rgb * Id * max(dot(N, L), 0.0);

	// normal mapping (in tangent space)
	vec3 t_V = normalize(vertexData.eyeDir);
	vec3 t_L = normalize(vertexData.lightDir);
	vec3 trice_t_N = normalize(texture(trice_normal, vertexData.texcoord).rgb * 2.0 - vec3(1.0));

	vec3 t_R = reflect(-t_L, trice_t_N);

	if(obj_id == 0)
	{
		if(default_tex == 0) 
		{
			if(texture(tex, vertexData.texcoord).a < 0.1) discard;
			fragColor = vec4( Ka * Ia + tex_diffuse + Ks * Is * pow(max(dot(R, V), 0.0), specular_power) , 1.0f);
		}
		else if(default_tex == 1) fragColor = vec4( Ka * Ia + Kd * Id * max(dot(N, L), 0.0) + Ks * Is * pow(max(dot(R, V), 0.0), specular_power) , 1.0f);
	}
	else if(obj_id == 1)
	{
		if(normal_mapping_flag == 1) fragColor = vec4( Ka * Ia + Kd * Id * max(dot(trice_t_N, t_L), 0.0) + Ks * Is * pow(max(dot(t_R, t_V), 0.0) , specular_power), 1.0f);
		else if(normal_mapping_flag == 0) fragColor = vec4( Ka * Ia + Kd * Id * max(dot(N, L), 0.0) + Ks * Is * pow(max(dot(R, V), 0.0), specular_power) , 1.0f);
	}
}