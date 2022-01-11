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

in VertexData
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

out vec4 fragColor;

float specular_power = 225.0;
vec3 Ia = vec3(0.2);
vec3 Id = vec3(0.7);
vec3 Is = vec3(0.1);

void main()
{
	// pure phong shading
	vec3 N = normalize(vertexData.N);
	vec3 L = normalize(vertexData.L);
	vec3 V = normalize(vertexData.V);

	vec3 R = reflect(-L, N);

	vec3 tex_diffuse = texture(tex, vertexData.texcoord).rgb * Id * max(dot(N, L), 0.0);
	// float shadow_factor = max(0.2, textureProj(shadow_tex, vertexData.shadow_coord));
	// float shadow_factor = textureProj(shadow_tex, vertexData.shadow_coord);

	// normal mapping (in tangent space)
	vec3 t_V = normalize(vertexData.eyeDir);
	vec3 t_L = normalize(vertexData.lightDir);
	vec3 trice_t_N = normalize(texture(trice_normal, vertexData.texcoord).rgb * 2.0 - vec3(1.0));

	vec3 t_R = reflect(-t_L, trice_t_N);

	vec3 ambient, diffuse, specular;
	if(obj_id == 0)
	{
		if(default_tex == 0) // Ka = map_Kd for this project
		{
			if(texture(tex, vertexData.texcoord).a < 0.1) discard;
			ambient = texture(tex, vertexData.texcoord).rgb * Ia;
			diffuse = tex_diffuse;
			specular = Ks * Is * pow(max(dot(R, V), 0.0), specular_power);
		}
		else if(default_tex == 1) 
		{
			ambient = Ka * Ia;
			diffuse = Kd * Id * max(dot(N, L), 0.0);
			specular = Ks * Is * pow(max(dot(R, V), 0.0), specular_power);
		}
	}
	else if(obj_id == 1)
	{
		if(normal_mapping_flag == 1) 
		{
			ambient = Ka * Ia;
			diffuse = Kd * Id * max(dot(trice_t_N, t_L), 0.0);
			specular = Ks * Is * pow(max(dot(t_R, t_V), 0.0) , specular_power);
		}
		else if(normal_mapping_flag == 0) 
		{
			ambient = Ka * Ia;
			diffuse = Kd * Id * max(dot(N, L), 0.0);
			specular = Ks * Is * pow(max(dot(R, V), 0.0), specular_power);
		}
	}

	// fragColor = vec4(ambient, 1.0) + shadow_factor * vec4(diffuse + specular, 1.0);
	// fragColor = vec4(ambient, 1.0) + vec4(diffuse + specular, 1.0);
	// fragColor = vec4(textureProj(shadow_tex, vertexData.shadow_coord));

	// float bias = 0.005;
	float bias = 0.005 * tan(acos( dot(N, L) ));
	bias = clamp(bias, 0.0, 0.01);
	float visibility = 1.0;
	if ( texture( shadow_tex, vertexData.shadow_coord.xy ).z < vertexData.shadow_coord.z - bias ) {
		visibility = 0.2;
	}
	fragColor = vec4(ambient, 1.0) + visibility * vec4(diffuse + specular, 1.0);
}