#version 420

in VertexData
{
	vec3 v_normal;
	vec3 u_normal;
	vec3 view;
} vertexData;

out vec4 fragColor;

vec3 light_pos = vec3(-2.51449f, 0.477241f, -1.21263f);
vec4 color;
  
void main()
{
	vec3 vN = normalize(vertexData.u_normal);
	vec3 uN = normalize(vertexData.v_normal);

	vec3 vL = normalize(light_pos - vertexData.view);
	vec3 uL = normalize(light_pos);
	float intensity = pow(max(0.0, dot(uN, uL)), 5.0);	float viewProduct = dot(vN, vL);
    if(intensity > 0.4) {
		color = vec4(233.0/255, 196.0/255, 106.0/255, 1.0);
    }
	else if(intensity > 0.2) {
		color = vec4(244.0/255, 162.0/255, 97.0/255, 1.0);
    }
	else if(intensity > 0.01) {
		color = vec4(231.0/255, 111.0/255, 81.0/255, 1.0);
    }
    else if(intensity > 0.001) {
		color = vec4(42.0/255, 157.0/255, 143.0/255, 1.0);
    }
    else {
		color = vec4(38.0/255, 70.0/255, 83.0/255, 1.0);
    }

	if(viewProduct <= 0) {
		color = vec4(13.0/255, 28.0/255, 33.0/255, 1.0);
	}
	
	fragColor = color;
	//fragColor = color * (intensity * 0.8 + 0.2);
}