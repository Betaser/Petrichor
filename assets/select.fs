#version 330

in vec2 fragTexCoord;

uniform float time;
// Should be inputs, but not important right now
// 0 to 1
const float density = 0.8;
// 0 to x
// Pretty sure we just want it to be 0
const float margin = 0.0;
// x - 1 = max(x, y)
const vec2 strokeRectDims = vec2(0.3, 0.05);

// A default input
uniform sampler2D texture0;
uniform ivec2 dims;

out vec4 finalColor;

vec2 getCenter(float pathX, float inMargin, float horzLength, float vertLength, out vec2 start, out vec2 end) {
    float pathN;
    if (pathX <= horzLength) {
        // left to right
        pathN = pathX / horzLength;
        start = vec2(inMargin, inMargin);
        end = vec2(horzLength + inMargin, inMargin);
    } else if (pathX <= horzLength + vertLength) {
        pathN = (pathX - horzLength) / vertLength;
        start = vec2(horzLength + inMargin, inMargin);
        end = vec2(horzLength + inMargin, vertLength + inMargin);
    } else if (pathX <= 2.0 * horzLength + vertLength) {
        pathN = (pathX - horzLength - vertLength) / horzLength;
        start = vec2(horzLength + inMargin, vertLength + inMargin);
        end = vec2(inMargin, vertLength + inMargin);
    } else {
        pathN = (pathX - 2.0 * horzLength - vertLength) / vertLength;
        start = vec2(inMargin, vertLength + inMargin);
        end = vec2(inMargin, inMargin);
    }
  
    vec2 center = start + (end - start) * pathN;
    return center;
}

void main() {
    finalColor = vec4(0);

    vec2 coord = fragTexCoord * vec2(dims);

    float smallerHalf = min(dims.x, dims.y) * 0.5;
    float inMargin = margin * smallerHalf;

    vec2 rectDims = strokeRectDims * smallerHalf;
    if (rectDims.x > rectDims.y) {
        rectDims = rectDims.yx;
    }

    vec2 uv = fragTexCoord;

    float longRectDim = rectDims.y;
    // for fun, let's sin vary strokeRectDims
    rectDims.x *= 1.0 + (0.5 * sin(time * 3.0) + 0.5);

    float horzLength = dims.x - 2.0 * inMargin;
    float vertLength = dims.y - 2.0 * inMargin;

    float pathLength = 2.0 * (horzLength + vertLength);
    float speed = pathLength / 60.0;

    int N = int(density * (pathLength / longRectDim * 0.5));

    vec2 mid = dims / 2.0;

    // Cap the locations at the bounds
    float border = inMargin;
    if (abs(coord.x - mid.x) > mid.x - border ||
        abs(coord.y - mid.y) > mid.y - border) {
        return;
    }

    for (int i = 0; i < N; i++) {
        // Black selection
        vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

        float offset = pathLength * (float(i) / float(N));

        float pathX = time * speed + offset;
        float back = pathX - longRectDim;
        float front = pathX + longRectDim;

        pathX = pathX - floor(pathX / pathLength) * pathLength;
        back = back - floor(back / pathLength) * pathLength;
        front = front - floor(front / pathLength) * pathLength;

        // back
        {
            vec2 start;
            vec2 end;
            vec2 backCenter = getCenter(back, inMargin, horzLength, vertLength, start, end);

            vec2 rDims = rectDims;

            if ((0.0 <= back && back <= horzLength) ||
                (horzLength + vertLength <= back && back <= 2.0 * horzLength + vertLength)) {
                rDims = rDims.yx;
            }

            vec2 frontCenter = backCenter + 2.0 * longRectDim * normalize(end - start);
            vec2 center = (backCenter + frontCenter) / 2.0;

            if (abs(coord.x - center.x) <= rDims.x &&
                abs(coord.y - center.y) <= rDims.y) {
                finalColor = color;
                break;
            }
        }

        // front
        {
            vec2 start;
            vec2 end;
            vec2 frontCenter = getCenter(front, inMargin, horzLength, vertLength, start, end);

            vec2 rDims = rectDims;

            if ((0.0 <= front && front <= horzLength) ||
                (horzLength + vertLength <= front && front <= 2.0 * horzLength + vertLength)) {
                rDims = rDims.yx;
            }

            vec2 backCenter = frontCenter - 2.0 * longRectDim * normalize(end - start);
            vec2 center = (backCenter + frontCenter) / 2.0;

            if (abs(coord.x - center.x) <= rDims.x &&
                abs(coord.y - center.y) <= rDims.y) {
                finalColor = color;
                break;
            }
        }
    }
}

/*
shadertoy
// 0 to 1
const float density = 0.8;
// 0 to x
const float margin = 0.3;
// x - 1 = max(x, y)
const vec2 strokeRectDims = vec2(0.3, 0.05);

vec2 getCenter(float pathX, float inMargin, float horzLength, float vertLength, 
out vec2 start, out vec2 end)
{
    float pathN;
    if (pathX <= horzLength) {
        // left to right
        pathN = pathX / horzLength;
        start = vec2(inMargin, inMargin);
        end = vec2(horzLength + inMargin, inMargin);
    } else if (pathX <= horzLength + vertLength) {
        pathN = (pathX - horzLength) / vertLength;
        start = vec2(horzLength + inMargin, inMargin);
        end = vec2(horzLength + inMargin, vertLength + inMargin);
    } else if (pathX <= 2.0 * horzLength + vertLength) {
        pathN = (pathX - horzLength - vertLength) / horzLength;
        start = vec2(horzLength + inMargin, vertLength + inMargin);
        end = vec2(inMargin, vertLength + inMargin);
    } else {
        pathN = (pathX - 2.0 * horzLength - vertLength) / vertLength;
        start = vec2(inMargin, vertLength + inMargin);
        end = vec2(inMargin, inMargin);
    }
  
    vec2 center = start + (end - start) * pathN;
    return center;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float smallerHalf = min(iResolution.x, iResolution.y) * 0.5;
    float inMargin = margin * smallerHalf;
    vec2 rectDims = strokeRectDims * smallerHalf;
    // not sure why but ensure that the smaller number is x.
    if (rectDims.x > rectDims.y) {
        rectDims = rectDims.yx;
    }
    
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

    // Time varying pixel color
    vec3 col = 0.5 + 0.5*cos(iTime+uv.xyx+vec3(0,2,4));
    
    float longRectDim = rectDims.y;
    float horzLength = iResolution.x - 2.0 * inMargin;
    float vertLength = iResolution.y - 2.0 * inMargin;
    
    float pathLength = 2.0 * (horzLength + vertLength);
    float speed = pathLength / 60.0;
    
    int N = int(density * (pathLength / longRectDim * 0.5));
    
    vec2 mid = iResolution.xy / 2.0;
    
    // Cap the locations at the bounds.
    // float border = inMargin - min(rectDims.x, rectDims.y);
    float border = inMargin;
    if (abs(fragCoord.x - mid.x) > mid.x - border ||
        abs(fragCoord.y - mid.y) > mid.y - border) {
        fragColor = vec4(col, 1.0);
        return;
    }
    
    for (int i = 0; i < N; i++) {
        vec3 color = vec3(0);

        float offset = pathLength * (float(i) / float(N));
        
        float pathX = iTime * speed + offset;
        float back = pathX - longRectDim;
        float front = pathX + longRectDim;
        
        pathX = pathX - floor(pathX / pathLength) * pathLength;
        back = back - floor(back / pathLength) * pathLength;
        front = front - floor(front / pathLength) * pathLength;
                
        // back        
        {
            vec2 start;
            vec2 end;
            vec2 backCenter = getCenter(back, inMargin, horzLength, vertLength, start, end);

            vec2 dims = rectDims;
                        
            if ((0.0 <= back && back <= horzLength) ||
                (horzLength + vertLength <= back && back <= 2.0 * horzLength + vertLength)) {
                dims = dims.yx;
            }

            vec2 frontCenter = backCenter + 2.0 * longRectDim * normalize(end - start);
            vec2 center = (backCenter + frontCenter) / 2.0;
    
            if (abs(fragCoord.y - center.y) <= dims.y &&
                abs(fragCoord.x - center.x) <= dims.x) {
                col = color;
            }
        }
        
        // front
        {
            vec2 start;
            vec2 end;
            vec2 frontCenter = getCenter(front, inMargin, horzLength, vertLength, start, end);

            vec2 dims = rectDims;
                        
            if ((0.0 <= front && front <= horzLength) ||
                (horzLength + vertLength <= front && front <= 2.0 * horzLength + vertLength)) {
                dims = dims.yx;
            }
            
            vec2 backCenter = frontCenter - 2.0 * longRectDim * normalize(end - start);
            vec2 center = (backCenter + frontCenter) / 2.0;
            
            if (abs(fragCoord.y - center.y) <= dims.y &&
                abs(fragCoord.x - center.x) <= dims.x) {
                col = color;
                break;
            }
        }
    }
    
    
    // Output to screen
    fragColor = vec4(col,1.0);
}
*/