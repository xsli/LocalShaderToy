// iDate 验证 Shader
// 在屏幕上显示当前日期和时间

// 简单的数字渲染函数
float digit(vec2 p, int n) {
    // 7段数码管样式
    n = clamp(n, 0, 9);
    
    vec2 s = step(vec2(0.0), p) - step(vec2(1.0, 1.5), p);
    if (s.x * s.y < 0.5) return 0.0;
    
    // 段定义: 上,右上,右下,下,左下,左上,中
    int segs[10] = int[10](
        0x7E, // 0: 1111110
        0x30, // 1: 0110000
        0x6D, // 2: 1101101
        0x79, // 3: 1111001
        0x33, // 4: 0110011
        0x5B, // 5: 1011011
        0x5F, // 6: 1011111
        0x70, // 7: 1110000
        0x7F, // 8: 1111111
        0x7B  // 9: 1111011
    );
    
    int seg = segs[n];
    float d = 0.0;
    
    // 上横
    if ((seg & 0x40) != 0) d += step(0.15, p.x) * step(p.x, 0.85) * step(1.35, p.y) * step(p.y, 1.5);
    // 右上竖
    if ((seg & 0x20) != 0) d += step(0.85, p.x) * step(0.75, p.y) * step(p.y, 1.35);
    // 右下竖
    if ((seg & 0x10) != 0) d += step(0.85, p.x) * step(0.15, p.y) * step(p.y, 0.75);
    // 下横
    if ((seg & 0x08) != 0) d += step(0.15, p.x) * step(p.x, 0.85) * step(0.0, p.y) * step(p.y, 0.15);
    // 左下竖
    if ((seg & 0x04) != 0) d += step(p.x, 0.15) * step(0.15, p.y) * step(p.y, 0.75);
    // 左上竖
    if ((seg & 0x02) != 0) d += step(p.x, 0.15) * step(0.75, p.y) * step(p.y, 1.35);
    // 中横
    if ((seg & 0x01) != 0) d += step(0.15, p.x) * step(p.x, 0.85) * step(0.7, p.y) * step(p.y, 0.8);
    
    return d;
}

// 渲染一个数字
float printDigit(vec2 uv, vec2 pos, int n) {
    vec2 p = (uv - pos) * 8.0;
    return digit(p, n);
}

// 渲染两位数
float printTwoDigits(vec2 uv, vec2 pos, int val) {
    float d = 0.0;
    d += printDigit(uv, pos, val / 10);
    d += printDigit(uv, pos + vec2(0.15, 0.0), val % 10);
    return d;
}

// 渲染四位数
float printFourDigits(vec2 uv, vec2 pos, int val) {
    float d = 0.0;
    d += printDigit(uv, pos, val / 1000);
    d += printDigit(uv, pos + vec2(0.15, 0.0), (val / 100) % 10);
    d += printDigit(uv, pos + vec2(0.30, 0.0), (val / 10) % 10);
    d += printDigit(uv, pos + vec2(0.45, 0.0), val % 10);
    return d;
}

// 渲染冒号
float printColon(vec2 uv, vec2 pos) {
    vec2 p = (uv - pos) * 16.0;
    float d = 0.0;
    d += step(length(p - vec2(0.5, 1.0)), 0.3);
    d += step(length(p - vec2(0.5, 2.0)), 0.3);
    return d;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    
    // 背景渐变
    vec3 col = mix(vec3(0.1, 0.1, 0.2), vec3(0.2, 0.1, 0.3), uv.y);
    
    // 从 iDate 提取数据
    int year = int(iDate.x);
    int month = int(iDate.y);
    int day = int(iDate.z);
    int totalSeconds = int(iDate.w);
    
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    
    // 渲染日期: YYYY-MM-DD
    float d = 0.0;
    d += printFourDigits(uv, vec2(0.15, 0.6), year);
    d += printTwoDigits(uv, vec2(0.52, 0.6), month);
    d += printTwoDigits(uv, vec2(0.75, 0.6), day);
    
    // 渲染时间: HH:MM:SS
    d += printTwoDigits(uv, vec2(0.25, 0.3), hours);
    d += printColon(uv, vec2(0.50, 0.3));
    d += printTwoDigits(uv, vec2(0.55, 0.3), minutes);
    d += printColon(uv, vec2(0.80, 0.3));
    d += printTwoDigits(uv, vec2(0.85, 0.3), seconds);
    
    // 数字颜色 - 青色
    vec3 digitColor = vec3(0.3, 1.0, 0.9);
    col = mix(col, digitColor, d);
    
    // 月份验证提示 (左上角)
    // 如果月份在 1-12 范围内显示绿色，否则红色
    vec2 indicatorPos = vec2(0.05, 0.9);
    float indicator = step(length(uv - indicatorPos), 0.03);
    vec3 indicatorColor = (month >= 1 && month <= 12) ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    col = mix(col, indicatorColor, indicator);
    
    fragColor = vec4(col, 1.0);
}
