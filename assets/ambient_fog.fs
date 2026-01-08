#version 330

in vec2 fragTexCoord;

uniform vec2 dims;

out vec4 finalColor;

void main() {
	finalColor = vec4(0);

	vec2 origin = vec2(0.5);
	
	vec2 out_v = fragTexCoord - origin;
	// Let's make it a circle-based algo instead of a rectangle
	out_v *= dims / max(dims.x, dims.y);

	// Edges should be 1.0 I think
	vec3 fogColor = vec3(0.9, 0.7, 0.9);
	float opacity = mix(0.3, 0.03, length(out_v));
	finalColor = vec4(fogColor, opacity);
}