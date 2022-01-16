#version 410

in VS_OUT
{
	vec3 position;
	vec2 texcoord;
	vec3 normal;
} fs_in;

uniform sampler2D tex;
uniform int default_tex;
uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;

layout (location = 0) out vec4 frag_position;
layout (location = 1) out vec4 frag_normal;
layout (location = 2) out vec4 frag_ambient;
layout (location = 3) out vec4 frag_diffuse;
layout (location = 4) out vec4 frag_specular;

void main()
{
	frag_position = vec4(fs_in.position, 1.0);
	frag_normal = vec4(normalize(fs_in.normal), 0.0);
	frag_ambient = vec4(Ka, 1.0);
	if(default_tex == 0) {
		if(texture(tex, fs_in.texcoord).a < 0.1) discard;
		frag_diffuse = texture(tex, fs_in.texcoord);
	}
	else if(default_tex == 1)
		frag_diffuse = vec4(Kd, 1.0);
	frag_specular = vec4(Ks, 1.0);
}