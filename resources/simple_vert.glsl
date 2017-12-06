#version  330 core
layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec3 vertNor;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

out vec3 wNor;
out vec3 wPos;

void main()
{
	gl_Position = P * V * M * vertPos;
	wNor = (M * vec4(vertNor, 0.0)).xyz;
	wPos = -1 * (M * vertPos).xyz;

	wNor = normalize(wNor);
	wPos = normalize(wPos);
}
