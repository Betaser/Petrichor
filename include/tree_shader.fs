// For now we just make the area go from a color input to brown

#version 330

// in vec3 vertexPos;
in vec2 fragTexCoord;
// in vec4 fragColor;

const int MAX = 100;
uniform int N;
uniform vec2 pt1s[MAX];
uniform vec2 pt2s[MAX];
uniform vec2 pt3s[MAX];
uniform vec2 pt4s[MAX];

uniform sampler2D tex;
uniform vec2 btmLefts[MAX];
uniform vec2 topRights[MAX];

uniform vec4 color;

out vec4 finalColor;

vec2 perp(vec2 v) {
    float vx = v.x;
    v.x = v.y;
    v.y = -vx;
    return v;
}

float distPtFromLine(vec2 point, vec2 line[2]) {
    vec2 toPoint = line[0] - point;
    vec2 outV = perp(line[1] - line[0]);
    return abs(dot(toPoint, outV) / length(outV));
}

void main() {
    float x = fragTexCoord.x;
    finalColor = vec4(0);

    for (int i = 0; i < N; i++) {
        // check if pixel in trapezoid
        vec2 pts[4] = { pt1s[i], pt2s[i], pt3s[i], pt4s[i] };

        bool inBranch = true;
        for (int j = 0; j < 4; j++) {
            vec2 a = pts[j];
            vec2 b = pts[(j + 1) % 4];
            vec3 v1 = vec3(a - b, 0);
            vec3 v2 = vec3(b - fragTexCoord.xy, 0);
            if (cross(v1, v2).z > 0.0) {
                inBranch = false;
                break;
            }
        }
        if (inBranch) {
            // Then we get the normalized x and y for the trapezoid

            vec2 arr[2];
            arr[0] = pts[0]; arr[1] = pts[1];
            float s0Dist = distPtFromLine(fragTexCoord.xy, arr);
            arr[0] = pts[0]; arr[1] = pts[3];
            float s1Dist = distPtFromLine(fragTexCoord.xy, arr);
            arr[0] = pts[2]; arr[1] = pts[3];
            float s2Dist = distPtFromLine(fragTexCoord.xy, arr);
            arr[0] = pts[1]; arr[1] = pts[2];
            float s3Dist = distPtFromLine(fragTexCoord.xy, arr);

            vec2 xy = vec2(s1Dist / (s1Dist + s3Dist), s0Dist / (s0Dist + s2Dist));

            // Now scale x to the cylindrical curve that the branch segment has
            float pi = 3.1415926535;
            // Below is very subtle, kinda can't see it actually
            xy.x = (asin(2.0 * (xy.x - 0.5)) / pi) + 0.5;

            // vec4 brown = vec4(2.0 * float(i) / N, 2.0 * float(i) / N, 0.0, 1.0);
            // finalColor = mix(color, brown, xy.x);
            // finalColor = mix(finalColor, vec4(1.0, 0.0, 0.0, 1.0), xy.y);
            vec2 regionXy = (topRights[i] - btmLefts[i]) * xy + btmLefts[i];
            // finalColor = vec4(regionXy.x, regionXy.y, 0, 1);

            ivec2 texelXy = ivec2(regionXy * textureSize(tex, 0));
            // ivec2 texelXy = ivec2(regionXy * textureSize(tex, 0));
            finalColor = texelFetch(tex, texelXy, 0);
            // finalColor = mix(finalColor, brown, xy.x);
            break;
        }
    }
}