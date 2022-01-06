#version 420

uniform sampler2D tex;
uniform sampler2D trice_normal;
uniform int default_tex;
uniform int obj_id;
uniform int normal_mapping_flag;

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

vec3 trice_albedo = vec3(0.243, 0.18, 0.141);
vec3 scene_albedo = vec3(1.0);
vec3 specular_albedo = vec3(1.0);
float specular_power = 128.0;
vec3 ambient = vec3(0.1, 0.1, 0.1);

void main()
{
	// pure phong shading
	vec3 N = normalize(vertexData.N);
	vec3 L = normalize(vertexData.L);
	vec3 V = normalize(vertexData.V);

	vec3 R = reflect(-L, N);

	vec3 scene_diffuse = max(dot(N, L), 0.0) * scene_albedo;
	vec3 tex_diffuse = max(dot(N, L), 0.0) * texture(tex, vertexData.texcoord).rgb;
	vec3 trice_diffuse = max(dot(N, L), 0.0) * trice_albedo;
	vec3 specular = pow(max(dot(R, V), 0.0), specular_power) * specular_albedo;

	// normal mapping (in tangent space)
	vec3 t_V = normalize(vertexData.eyeDir);
	vec3 t_L = normalize(vertexData.lightDir);
	vec3 trice_t_N = normalize(texture(trice_normal, vertexData.texcoord).rgb * 2.0 - vec3(1.0));

	vec3 t_R = reflect(-t_L, trice_t_N);

	vec3 trice_t_diffuse = max(dot(trice_t_N, t_L), 0.0) * trice_albedo;
	vec3 trice_specular = max( pow( dot(t_R, t_V), 20.0), 0.0 ) * specular_albedo;

	if(obj_id == 0)
	{
		if(default_tex == 0) fragColor = vec4(ambient + tex_diffuse + specular, 1.0f);
		else if(default_tex == 1) fragColor = vec4(ambient + scene_diffuse + specular, 0.0f);
	}
	else if(obj_id == 1)
	{
		if(normal_mapping_flag == 1) fragColor = vec4(ambient + trice_t_diffuse + trice_specular, 1.0f);
		else if(normal_mapping_flag == 0) fragColor = vec4(ambient + trice_diffuse + specular, 1.0f);;
		// fragColor = texture(trice_normal, vertexData.texcoord);
	}
}