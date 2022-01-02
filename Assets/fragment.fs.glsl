#version 420

out vec4 fragColor;

uniform sampler2D tex;

in VertexData
{
	vec3 normal;
    vec2 texcoord;
} vertexData;

void main()
{
	vec3 texColor = texture(tex, vertexData.texcoord).rgb;
	fragColor = vec4(texColor, 1.0);
	// fragColor = vec4(1.0);
}