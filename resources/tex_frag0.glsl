#version 330 core

uniform sampler2D Texture0;

in vec2 vTexCoord;

out vec4 Outcolor;

void main() {
  	vec4 texColor0 = texture(Texture0, vTexCoord);

  	Outcolor = vec4(texColor0.r, texColor0.g, texColor0.b, 1);
}

