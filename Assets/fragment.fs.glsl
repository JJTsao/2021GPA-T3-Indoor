#version 420

out vec4 fragColor;

uniform sampler2D tex;
uniform int default_tex;
uniform int obj_id;

in VertexData
{
	vec3 normal;
    vec2 texcoord;
} vertexData;

vec3 trice_color = vec3(0.243f, 0.18f, 0.141f);

void main()
{
	if(obj_id == 0)
	{
		if(default_tex == 0) fragColor = texture(tex, vertexData.texcoord).rgba;
		else if(default_tex == 1) fragColor = vec4(vec3(0.8f), 1.0f);
	}
	else if(obj_id == 1)
	{
		fragColor = vec4(trice_color, 1.0f);
	}
}