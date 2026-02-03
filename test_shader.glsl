// Test Shadertoy shader - Plasma effect
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    vec2 p = uv * 2.0 - 1.0;
    p.x *= iResolution.x / iResolution.y;
    
    float t = iTime * 0.5;
    
    // Plasma effect
    float v = sin(p.x * 10.0 + t);
    v += sin((p.y * 10.0 + t) * 0.5);
    v += sin((p.x * 10.0 + p.y * 10.0 + t) * 0.5);
    
    float cx = p.x + sin(t / 3.0) * 0.5;
    float cy = p.y + cos(t / 2.0) * 0.5;
    v += sin(sqrt(cx * cx + cy * cy + 1.0) * 10.0 + t);
    
    v /= 2.0;
    
    // Color
    vec3 col = vec3(sin(v * 3.14159), sin(v * 3.14159 + 2.094), sin(v * 3.14159 + 4.188));
    col = col * 0.5 + 0.5;
    
    fragColor = vec4(col, 1.0);
}
