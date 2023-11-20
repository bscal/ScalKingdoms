#version 430 core

in vec2 fragTexCoord;

uniform sampler2D texture0; // World Map

uniform vec4 ambientColor;

out vec4 finalColor;

void main()
{
	vec4 texColor = texture(texture0, fragTexCoord);
	finalColor = texColor * ambientColor;
}