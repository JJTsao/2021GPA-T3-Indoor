#version 410

uniform sampler2D originView;
uniform sampler2D brightView;
uniform sampler2D position_map;
uniform sampler2D normal_map;
uniform sampler2D ambient_map;
uniform sampler2D diffuse_map;
uniform sampler2D specular_map;
uniform int deferred_enabled;
uniform int deferred_flag;

in vec2 tex_coord;

out vec4 fragColor;

void main()
{
	if(deferred_enabled == 0) {
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
	} else if(deferred_enabled == 1) {
		switch(deferred_flag) {
			case 0:
				fragColor = texture(position_map, tex_coord);
				break;
			case 1:
				fragColor = texture(normal_map, tex_coord);
				break;
			case 2:
				fragColor = texture(ambient_map, tex_coord);
				break;
			case 3:
				fragColor = texture(diffuse_map, tex_coord);
				break;
			case 4:
				fragColor = texture(specular_map, tex_coord);
				break;
		}
	}
}