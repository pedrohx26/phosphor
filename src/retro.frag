// SPDX-License-Identifier: GPL-2.0-or-later
// retro-term — KWin 6 GLSL 1.40 Fragment Shader
//
// KWin's ShaderManager (ShaderTrait::MapTexture) provides:
//   in vec2 texcoord0;        — normalised [0,1] UV for the window texture
//   uniform sampler2D sampler; — the window framebuffer texture (unit 0)
//
// All uniforms below are set each frame by RetroTermEffect::paintWindow().

#version 140

in  vec2 texcoord0;
out vec4 fragColor;

uniform sampler2D sampler;
uniform vec2  resolution;
uniform float time;

// Phosphor
uniform int   phosphorType;
uniform float phosphorAgeing;
uniform float colorTemperature;
uniform float phosphorPersistence;

// Geometry
uniform float screenCurvature;
uniform float vignetteIntensity;
uniform float ambientReflection;

// Scanlines
uniform int   rasterizationMode;
uniform float scanlinesIntensity;
uniform float scanlinesSharpness;

// Bloom / Glow
uniform float bloom;
uniform float glowingLine;
uniform float brightness;
uniform float contrast;

// Noise / Sync
uniform float staticNoise;
uniform float jitter;
uniform int   syncMode;
uniform float horizontalSync;
uniform float flickering;
uniform float ghostingIntensity;

// Chroma / appearance
uniform float chromaColor;
uniform float saturationColor;
uniform float rbgShift;
uniform float characterSmearing;
uniform float burnIn;

// Animations
uniform float warmupProgress;
uniform float degaussProgress;

// ─────────────────────────────────────────────────────────────────────────────
// Utility
// ─────────────────────────────────────────────────────────────────────────────
float hash(vec2 p) {
    p = fract(p * vec2(443.8975, 397.2973));
    p += dot(p.xy, p.yx + 19.19);
    return fract(p.x * p.y);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i),             hash(i + vec2(1.0, 0.0)), f.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x), f.y);
}

float rand(float s) { return fract(sin(s * 12.9898 + 78.233) * 43758.5453); }
float lum(vec3 c)   { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }

// ─────────────────────────────────────────────────────────────────────────────
// CRT Effects
// ─────────────────────────────────────────────────────────────────────────────

vec2 barrel(vec2 uv) {
    if (screenCurvature < 0.001) return uv;
    vec2 cc = uv - 0.5;
    return uv + cc * dot(cc, cc) * screenCurvature * 0.15;
}

vec3 phosphorTint(vec3 grey) {
    vec3 t;
    if      (phosphorType == 0) t = vec3(0.20, 1.00, 0.20);
    else if (phosphorType == 1) t = vec3(1.00, 0.65, 0.08);
    else if (phosphorType == 2) t = vec3(0.95, 0.95, 0.90);
    else                        t = vec3(0.18, 0.90, 0.35);
    t = mix(t, vec3(0.80, 0.72, 0.20), phosphorAgeing * 0.6);
    return grey * t;
}

vec3 colorTemp(vec3 col) {
    float t = clamp((colorTemperature - 3000.0) / 6300.0, 0.0, 1.0);
    return clamp(col * mix(vec3(1.0, 0.75, 0.45), vec3(0.85, 0.90, 1.10), t), 0.0, 1.0);
}

float scanlines(vec2 uv) {
    if (rasterizationMode == 0) return 1.0;
    float ph = fract(uv.y * resolution.y);
    if (rasterizationMode == 1) {
        float l = smoothstep(0.0, scanlinesSharpness + 0.01, ph)
                * smoothstep(1.0, 1.0 - scanlinesSharpness - 0.01, ph);
        return mix(1.0, l, scanlinesIntensity);
    }
    if (rasterizationMode == 2) {
        float px = smoothstep(0.0, 0.4, fract(uv.x * resolution.x))
                 * smoothstep(1.0, 0.6, fract(uv.x * resolution.x));
        float py = smoothstep(0.0, 0.4, ph) * smoothstep(1.0, 0.6, ph);
        return mix(1.0, px * py, scanlinesIntensity);
    }
    return 1.0;
}

vec3 applyBloom(vec2 uv) {
    if (bloom < 0.001) return vec3(0.0);
    float b = bloom * 0.012;
    vec3  acc = vec3(0.0);
    float w[13];
    w[0]=0.095; w[1]=w[2]=0.090; w[3]=w[4]=0.075;
    w[5]=w[6]=0.055; w[7]=w[8]=0.035; w[9]=w[10]=0.018; w[11]=w[12]=0.008;
    vec2 o[13];
    o[0]=vec2(0);
    o[1]=vec2(b,0); o[2]=vec2(-b,0); o[3]=vec2(0,b); o[4]=vec2(0,-b);
    o[5]=vec2(b,b); o[6]=vec2(-b,-b);
    o[7]=vec2(2.0*b,0); o[8]=vec2(-2.0*b,0);
    o[9]=vec2(0,2.0*b); o[10]=vec2(0,-2.0*b);
    o[11]=vec2(b,2.0*b); o[12]=vec2(-b,-2.0*b);
    for (int i = 0; i < 13; i++)
        acc += texture(sampler, uv + o[i]).rgb * w[i];
    return acc;
}

vec2 syncDistort(vec2 uv) {
    if (syncMode == 0 || horizontalSync < 0.001) return uv;
    float s = floor(uv.y * resolution.y);
    if (syncMode == 1)
        return uv + vec2(sin(time * 2.3 + s * 0.1) * horizontalSync * 0.008, 0.0);
    if (syncMode == 2) {
        float ph = fract(uv.y + fract(time * 0.12));
        return uv + vec2(ph < 0.02 ? horizontalSync * 0.05 * rand(s + time) : 0.0, 0.0);
    }
    return uv + vec2((rand(s + floor(time * 30.0)) - 0.5) * horizontalSync * 0.004, 0.0);
}

vec3 applyWarmup(vec3 col, vec2 uv) {
    if (warmupProgress >= 1.0) return col;
    float p  = warmupProgress;
    float br = mix(0.004, 0.5, smoothstep(0.0, 0.6, p));
    float bm = smoothstep(br + 0.01, br, abs(uv.y - 0.5));
    float fd = mix(0.0, 1.0, smoothstep(0.1, 0.9, p));
    return mix(col * vec3(0.7, 0.85, 1.0), col, smoothstep(0.5, 1.0, p)) * bm * fd;
}

vec3 applyDegauss(vec3 col, vec2 uv) {
    if (degaussProgress <= 0.001 || degaussProgress >= 1.0) return col;
    float p  = degaussProgress;
    vec2  d  = uv - 0.5;
    float r  = length(d);
    float sw = sin(atan(d.y, d.x) * 8.0 + time * 20.0) * (1.0 - p) * 0.04;
    vec2  suv = clamp(uv + normalize(d) * sw, 0.0, 1.0);
    float sp  = abs(sin(time * 12.0 + r * 15.0)) * (1.0 - p);
    vec3  sc  = texture(sampler, suv).rgb;
    float h   = fract(r * 3.0 + time * 2.0 + p);
    vec3  rb  = 0.5 + 0.5 * cos(6.2832 * (h + vec3(0.0, 0.333, 0.667)));
    return mix(col, mix(sc, rb, sp * 0.4), (1.0 - p) * 0.8);
}

// ─────────────────────────────────────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────────────────────────────────────
void main() {
    vec2 uv = barrel(texcoord0);

    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    uv = syncDistort(uv);
    if (jitter > 0.001)
        uv += vec2((noise(vec2(uv.y * resolution.y, floor(time * 60.0))) - 0.5)
                   * jitter * 0.003, 0.0);
    uv = clamp(uv, 0.001, 0.999);

    // Chromatic aberration
    vec3 col;
    if (rbgShift > 0.001) {
        float sh = rbgShift * 0.004;
        col = vec3(texture(sampler, uv + vec2(sh,  0.0)).r,
                   texture(sampler, uv).g,
                   texture(sampler, uv - vec2(sh, 0.0)).b);
    } else {
        col = texture(sampler, uv).rgb;
    }

    // Character smearing
    if (characterSmearing > 0.001) {
        vec2 px = vec2(1.0 / resolution.x, 0.0);
        col = mix(col,
                  (texture(sampler, uv - px).rgb + col
                   + texture(sampler, uv + px).rgb) / 3.0,
                  characterSmearing);
    }

    col += applyBloom(uv) * bloom;

    if (ghostingIntensity > 0.001 && syncMode == 3)
        col += texture(sampler, uv + vec2(0.003, 0.0)).rgb * ghostingIntensity;

    if (phosphorPersistence > 0.001) {
        vec3 prev = texture(sampler, uv + vec2(0.0, 1.0 / resolution.y)).rgb * 0.6;
        col = mix(col, col + prev * phosphorPersistence, 0.35);
    }

    // Phosphor tint + colour temperature + saturation
    float g = lum(col);
    col = mix(phosphorTint(vec3(g)), col, chromaColor);
    col = colorTemp(col);
    col = mix(vec3(lum(col)), col, 1.0 + saturationColor);

    // Contrast / brightness
    col = clamp((col - 0.5) * (contrast + 0.5) + 0.5 + (brightness - 0.5) * 0.4,
                0.0, 1.0);

    col *= scanlines(uv);

    if (staticNoise > 0.001)
        col += (noise(uv * resolution * 0.5
                + vec2(fract(time / 51.0), fract(time / 237.0)) * 500.0)
                - 0.5) * staticNoise * 0.15;

    if (flickering > 0.001) {
        float f = 0.9 + 0.1 * sin(time * 188.5)
                      + 0.05 * sin(time * 18.0)
                      + 0.02 * noise(vec2(time * 30.0, 0.0));
        col *= mix(1.0, f, flickering);
    }

    if (vignetteIntensity > 0.001) {
        vec2 c = uv - 0.5;
        col *= 1.0 - dot(c, c) * vignetteIntensity * 2.5;
    }

    if (ambientReflection > 0.001) {
        vec2  hs = uv - vec2(0.85, 0.08);
        float sp = exp(-dot(hs, hs) * 20.0);
        vec2  ed = min(uv, 1.0 - uv);
        col += (sp * 0.4 + (1.0 - smoothstep(0.0, 0.08, min(ed.x, ed.y))) * 0.3)
               * ambientReflection;
    }

    if (burnIn > 0.001)
        col += col * (1.0 - length(uv - 0.5) * 0.8) * burnIn * 0.08;

    if (glowingLine > 0.001)
        col += vec3(exp(-abs(fract(uv.y * resolution.y / 25.0) - 0.5) * 80.0)
                    * glowingLine * 0.06);

    col = applyWarmup(col, uv);
    col = applyDegauss(col, uv);

    fragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
