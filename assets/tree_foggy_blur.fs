// I'm not even sure if I can apply a completely new shader on top of an existing shader, let's see

#version 330

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 dims;
uniform float distFromCam;
uniform float collisionDist;
const float MAX_DIST_FROM_CAM = 150;

void main() {
	// There is a weird jump to being in focus, so mess with numbers.
	float weights3x3[9] = float[9](1, 2, 1, 2, 10, 2, 1, 2, 1);
	float totalWeight = 0;
	for (int i = 0; i < 9; i++) {
		totalWeight += weights3x3[i];
	}
	for (int i = 0; i < 9; i++) {
		weights3x3[i] /= totalWeight;
	}

	// Suppose 60 is the collision dist
	float distRatio = min(1, max(0, distFromCam - collisionDist) / MAX_DIST_FROM_CAM);
	float blurDist = mix(0.0, 0.05, distRatio);
	// Make a few percent before the distRatio "pop"
	if (distRatio < 0.15) {
		distRatio = 0;
	}

	// Square shaped sample? idk
	vec4 total = vec4(0);
	for (int i = -1; i < 2; i++) {
		for (int j = -1; j < 2; j++) {
			vec2 offset = vec2(i, j) * blurDist;
			vec2 uv = fragTexCoord + offset;
			if (!(0 <= uv.x && uv.x <= 1 && 0 <= uv.y && uv.y <= 1))
				continue;
			vec4 sample = texture(texture0, fragTexCoord + offset);

			float weight = weights3x3[(i + 1) * 3 + (j + 1)];
			total += weight * sample;
		}
	}

	finalColor = total;
	// Tint stuff far away
	float tintAmt = mix(0.0, 0.4, distRatio);
	finalColor = mix(total, vec4(0.9, 0.7, 0.9, total.a), tintAmt);
}

void notMain() {
	float weights3x3[9] = float[9](1, 2, 1, 2, 4, 2, 1, 2, 1);
	for (int i = 0; i < 9; i++) {
		weights3x3[i] /= 16;
	}

	vec2 origin = vec2(0.5);
	vec2 out_v = fragTexCoord - origin;
	// Let's make it a circle-based algo instead of a rectangle
	out_v *= dims / max(dims.x, dims.y);
	float dist = length(out_v);
	float blurDist = dist < 0.2 ? 0 : mix(0.0, 0.01, dist);

	// Square shaped sample? idk
	vec4 total = vec4(0);
	for (int i = -1; i < 2; i++) {
		for (int j = -1; j < 2; j++) {
			vec2 offset = vec2(i, j) * blurDist;
			vec4 sample = texture(texture0, fragTexCoord + offset);

			float weight = weights3x3[(i + 1) * 3 + (j + 1)];
			total += weight * sample;
		}
	}
	finalColor = total;
}