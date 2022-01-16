#version 410

uniform sampler2D diffuse_tex;
uniform int default_tex;

in vec2 tex_coord;

out vec4 fragColor;

void main()
{
	if(default_tex == 0) {
		if(texture(diffuse_tex, tex_coord).a < 0.1)
			discard;
	}
	fragColor = vec4(vec3(gl_FragCoord.z), 1.0);
}