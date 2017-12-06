#version 330 core 
in vec3 wNor;
in vec3 wPos;

uniform vec3 lightPos;
uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;
uniform float shine;

out vec4 color;

void main()
{
	vec3 normal = normalize(wNor);
	vec3 lightDir = normalize(lightPos - wPos);
	vec3 lightColor = vec3(1.0, 1.0, 1.0);

	vec3 cameraDir = normalize(wPos);
	
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = diff * MatDif * lightColor;
	
	vec3 ambient = MatAmb * lightColor; 

	// Calculating Specular Light
	float specTerm = 0;
	
	vec3 halfVector = normalize(lightDir + cameraDir);
	
	specTerm = pow(max(dot(normal, halfVector), 0), shine);
	
	vec3 specular = MatSpec * specTerm * lightColor;

	vec3 result = (ambient + diffuse + specular);
	
	color = vec4(result, 1.0);
}
