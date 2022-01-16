#version 410

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 iv2tex_coord;

out vec2 tex_coord;

void main()
{
	tex_coord = iv2tex_coord;
	gl_Position = vec4(position, 0.0, 1.0);
}