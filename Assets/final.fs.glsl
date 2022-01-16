#version 410

uniform sampler2D originView;
uniform sampler2D brightView;

in vec2 tex_coord;

out vec4 fragColor;

void main()
{
	int half_size = 5;
	vec4 color_sum = vec4(0);
	for(int i = -half_size; i <= half_size; i++) {
		for(int j = -half_size; j <= half_size; j++) {
			ivec2 coord = ivec2(gl_FragCoord.xy) + ivec2(i, j);
			color_sum += texelFetch(brightView, coord, 0);
		}
	}
	int sampler_count = (half_size * 2 + 1) * (half_size * 2 + 1);
	vec4 gaussian_color = 1.5 * color_sum / sampler_count;

	fragColor = texture(originView, tex_coord).rgba + gaussian_color;
}