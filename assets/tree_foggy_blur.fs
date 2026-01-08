// I'm not even sure if I can apply a completely new shader on top of an existing shader, let's see

#version 330

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D tex;

void main() {
	finalColor = texture(tex, fragTexCoord);
	finalColor.r *= 0.7;
}