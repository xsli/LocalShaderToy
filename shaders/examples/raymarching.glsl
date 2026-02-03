// Raymarching Sphere - A simple raymarching example
// Demonstrates: iResolution, iTime, iMouse

#define MAX_STEPS 100
#define MAX_DIST 100.0
#define SURF_DIST 0.001

float sdSphere(vec3 p, float r) {
    return length(p) - r;
}

float sdBox(vec3 p, vec3 b) {
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float sdPlane(vec3 p) {
    return p.y;
}

float getDist(vec3 p) {
    float sphere = sdSphere(p - vec3(0.0, 1.0, 6.0), 1.0);
    float plane = sdPlane(p);
    
    float d = min(sphere, plane);
    return d;
}

float rayMarch(vec3 ro, vec3 rd) {
    float dO = 0.0;
    
    for (int i = 0; i < MAX_STEPS; i++) {
        vec3 p = ro + rd * dO;
        float dS = getDist(p);
        dO += dS;
        if (dO > MAX_DIST || dS < SURF_DIST) break;
    }
    
    return dO;
}

vec3 getNormal(vec3 p) {
    float d = getDist(p);
    vec2 e = vec2(0.001, 0.0);
    
    vec3 n = d - vec3(
        getDist(p - e.xyy),
        getDist(p - e.yxy),
        getDist(p - e.yyx));
    
    return normalize(n);
}

float getLight(vec3 p) {
    vec3 lightPos = vec3(0.0, 5.0, 6.0);
    lightPos.xz += vec2(sin(iTime), cos(iTime)) * 2.0;
    vec3 l = normalize(lightPos - p);
    vec3 n = getNormal(p);
    
    float dif = clamp(dot(n, l), 0.0, 1.0);
    
    // Shadow
    float d = rayMarch(p + n * SURF_DIST * 2.0, l);
    if (d < length(lightPos - p)) dif *= 0.1;
    
    return dif;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    
    // Mouse interaction
    vec3 ro = vec3(0.0, 2.0, 0.0);
    if (iMouse.z > 0.0) {
        ro.xz = vec2(sin(iMouse.x * 0.01), cos(iMouse.x * 0.01)) * 3.0;
    }
    
    vec3 rd = normalize(vec3(uv.x, uv.y - 0.2, 1.0));
    
    float d = rayMarch(ro, rd);
    
    vec3 col = vec3(0.0);
    
    if (d < MAX_DIST) {
        vec3 p = ro + rd * d;
        float dif = getLight(p);
        col = vec3(dif);
    }
    
    // Gamma correction
    col = pow(col, vec3(0.4545));
    
    fragColor = vec4(col, 1.0);
}
