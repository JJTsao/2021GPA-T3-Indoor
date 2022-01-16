#version 420

uniform sampler2D tex;
uniform sampler2D trice_normal;
uniform sampler2D shadow_tex;
uniform int default_tex;
uniform int obj_id;
uniform int normal_mapping_flag;
uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;

in vec3 eyeDir;
in vec3 t_eyeDir;

in vec3 dirLightDir;
in vec3 t_dirLightDir;
in vec3 pointLightDir;
in vec3 t_pointLightDir;
in vec4 shadow_coord;

in VertexData
{
	vec2 texcoord;
	vec3 normal;
} vertexData;

layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 brightFilterColor;

float specular_power = 225.0;
vec3 Ia = vec3(0.2);
vec3 Id = vec3(0.7);
vec3 Is = vec3(0.1);
vec3 bloom_Id = vec3(1.0);

vec4 CalcDirLight(vec3 lightDir, vec3 normal, vec3 viewDir, float shadowFactor, vec3 Kd)
{
	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 ambient, diffuse, specular;
	ambient = Kd * Ia;
	diffuse = Kd * Id * max(dot(normal, lightDir), 0.0);
	specular = Ks * Is * pow(max(dot(viewDir, reflectDir), 0.0), specular_power);
	return vec4(ambient + shadowFactor * (diffuse + specular), 1.0);
}

vec4 CalcPointLight(vec3 lightDir, vec3 normal, vec3 viewDir, float fa, vec3 Kd)
{
	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 ambient, diffuse, specular;
	ambient = Kd * Ia;
	diffuse = Kd * bloom_Id * max(dot(normal, lightDir), 0.0);
	specular = Ks * Is * pow(max(dot(viewDir, reflectDir), 0.0), specular_power);
	
	return vec4(ambient + fa * (diffuse + specular), 1.0);
}

void main()
{
	float bias = 0.0005 * tan(acos( dot(vertexData.normal, dirLightDir) ));
	float visibility = 1.0;
	bias = clamp(bias, 0.0, 0.0005);
	if ( texture( shadow_tex, shadow_coord.xy ).z < shadow_coord.z - bias ) {
		visibility = 0.2;
	}

	if(obj_id == 1 && normal_mapping_flag == 1) { // normal mapping (tangent space)
		vec3 dir_L = normalize(t_dirLightDir);
		vec3 point_L = normalize(t_pointLightDir);
		float d = length(pointLightDir);
		float attenuation = min( 10.0 / (1 + 2 * d + 2 * pow(d, 2)), 1);
		vec3 N = normalize(texture(trice_normal, vertexData.texcoord).rgb * 2.0 - vec3(1.0));
		vec3 V = normalize(t_eyeDir);
		fragColor += CalcDirLight(dir_L, N, V, visibility, Kd);
		fragColor += CalcPointLight(point_L, N, V, attenuation, Kd);
	} else if(obj_id == 0 && default_tex == 0) {
		if(texture(tex, vertexData.texcoord).a < 0.1) discard; // transparent pixels

		vec3 dir_L = normalize(dirLightDir);
		vec3 point_L = normalize(pointLightDir);
		float d = length(pointLightDir);
		float attenuation = min( 10.0 / (1 + 2 * d + 2 * pow(d, 2)), 1);
		vec3 N = normalize(texture(trice_normal, vertexData.texcoord).rgb * 2.0 - vec3(1.0));
		vec3 V = normalize(eyeDir);
		vec3 diffuse_Kd = texture(tex, vertexData.texcoord).rgb;
		fragColor += CalcDirLight(dir_L, N, V, visibility, diffuse_Kd);
		fragColor += CalcPointLight(point_L, N, V, attenuation, diffuse_Kd);
	} else if(obj_id == 2) { // Point Light Sphere
		vec4 bloomColor = vec4(1.0, 1.0, 0.8, 1.0);
		float brightness = (bloomColor.r + bloomColor.g + bloomColor.b) / 3.0f;
		brightFilterColor = bloomColor * brightness;
	} else {
		vec3 dir_L = normalize(dirLightDir);
		vec3 point_L = normalize(pointLightDir);
		float d = length(pointLightDir);
		float attenuation = min( 10.0 / (1 + 2 * d + 2 * pow(d, 2)), 1);
		vec3 N = normalize(vertexData.normal);
		vec3 V = normalize(eyeDir);
		fragColor += CalcDirLight(dir_L, N, V, visibility, Kd);
		fragColor += CalcPointLight(point_L, N, V, attenuation, Kd);
	}
}