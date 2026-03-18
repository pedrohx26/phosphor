// SPDX-License-Identifier: GPL-2.0-or-later
// retro-term — KWin 6 GLSL 1.40 Fragment Shader
//
// KWin's ShaderManager (ShaderTrait::MapTexture) provides:
//   in vec2 texcoord0;         — normalised [0,1] UV for the window texture
//   uniform sampler2D sampler; — the window framebuffer texture (unit 0)

#version 140

in  vec2 texcoord0;
out vec4 fragColor;

uniform sampler2D sampler;
uniform vec2  resolution;       // venstergrootte in pixels
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

// ── Pixel scaling ─────────────────────────────────────────────────────────────
// pixelScale:   0.0 = no scaling (modern), 1.0 = pixel-exact original
// targetRes:    original resolution of the historical system (e.g. 320x200)
// sampleMode:   0 = nearest-neighbour (hard block pixels)
//               1 = bilinear (soft, good for intermediate values)
//               2 = sharp-bilinear (sharp edges without aliasing, CRT-like)
uniform float pixelScale;
uniform vec2  targetRes;
uniform int   sampleMode;

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
        mix(hash(i),                  hash(i + vec2(1.0, 0.0)), f.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x), f.y);
}

float rand(float s) { return fract(sin(s * 12.9898 + 78.233) * 43758.5453); }
float lum(vec3 c)   { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }

// ─────────────────────────────────────────────────────────────────────────────
// Pixel scaling - core function
//
// Samples the texture using the selected sampling mode on a UV that can be
// quantized to the grid of the original resolution.
//
// Theory:
//   A CRT pixel is not a hard block. The electron beam has a Gaussian profile.
//   "Sharp bilinear" (mode 2) approximates this by linearly interpolating
//   inside a pixel cell while keeping edge transitions tight with smoothstep.
//   This yields a familiar CRT pixel look without aliasing.
// ─────────────────────────────────────────────────────────────────────────────

// Quantize uv to the targetRes grid with a transition zone controlled by
// pixelScale. At scale=0 -> original uv, at scale=1 -> fully quantized.
vec2 scaleUV(vec2 uv) {
    if (pixelScale < 0.001) return uv;

    // Effective resolution: interpolate between window resolution and targetRes
    vec2 effRes = mix(resolution, targetRes, pixelScale);

    if (sampleMode == 0) {
        // Nearest-neighbour: hard block pixels
        return (floor(uv * effRes) + 0.5) / effRes;
    }
    else if (sampleMode == 2) {
        // Sharp bilinear (CRT-like):
        // Compute position within the pixel in [0,1]
        vec2 pos   = uv * effRes;
        vec2 ipos  = floor(pos);
        vec2 fpos  = fract(pos);
        // Smoothstep keeps transitions tight around 0.5 while smooth in-cell
        float sharpness = mix(1.0, 6.0, pixelScale); // sharpness increases with scale
        vec2  curved = smoothstep(0.0, 1.0,
            fpos * sharpness - (sharpness * 0.5 - 0.5));
        curved = clamp(curved, 0.0, 1.0);
        return (ipos + curved) / effRes;
    }
    else {
        // Bilinear: use effective resolution as the sampling grid
        // (GPU bilinear filtering does the rest)
        vec2 pos  = uv * effRes;
        vec2 ipos = floor(pos);
        vec2 fpos = fract(pos);
        fpos = fpos * fpos * (3.0 - 2.0 * fpos); // smoothstep for a softer look
        return (ipos + fpos) / effRes;
    }
}

// Read texel with pixel scaling applied
vec4 sampleScaled(vec2 uv) {
    return texture(sampler, scaleUV(uv));
}

// ─────────────────────────────────────────────────────────────────────────────
// CRT effects
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

// Scanlines operate on effective resolution when pixelScale is active,
// so scanlines align with scaled pixels
float scanlines(vec2 uv) {
    if (rasterizationMode == 0) return 1.0;

    // Use effective resolution for scanline positioning
    float effY = mix(resolution.y, targetRes.y, pixelScale);
    float ph   = fract(uv.y * effY);

    if (rasterizationMode == 1) {
        float l = smoothstep(0.0, scanlinesSharpness + 0.01, ph)
                * smoothstep(1.0, 1.0 - scanlinesSharpness - 0.01, ph);
        return mix(1.0, l, scanlinesIntensity);
    }
    if (rasterizationMode == 2) {
        float effX = mix(resolution.x, targetRes.x, pixelScale);
        float px = smoothstep(0.0, 0.4, fract(uv.x * effX))
                 * smoothstep(1.0, 0.6, fract(uv.x * effX));
        float py = smoothstep(0.0, 0.4, ph) * smoothstep(1.0, 0.6, ph);
        return mix(1.0, px * py, scanlinesIntensity);
    }
    return 1.0;
}

// Bloom uses scaleUV so glow aligns with scaled pixels
vec3 applyBloom(vec2 uv) {
    if (bloom < 0.001) return vec3(0.0);
    float b = bloom * 0.012;
    vec3  acc = vec3(0.0);
    float w[13];
    w[0]=0.095; w[1]=w[2]=0.090; w[3]=w[4]=0.075;
    w[5]=w[6]=0.055; w[7]=w[8]=0.035; w[9]=w[10]=0.018; w[11]=w[12]=0.008;
    vec2 o[13];
    o[0]=vec2(0);
    o[1]=vec2(b,0);     o[2]=vec2(-b,0);    o[3]=vec2(0,b);    o[4]=vec2(0,-b);
    o[5]=vec2(b,b);     o[6]=vec2(-b,-b);
    o[7]=vec2(2.0*b,0); o[8]=vec2(-2.0*b,0);
    o[9]=vec2(0,2.0*b); o[10]=vec2(0,-2.0*b);
    o[11]=vec2(b,2.0*b);o[12]=vec2(-b,-2.0*b);
    for (int i = 0; i < 13; i++)
        acc += sampleScaled(uv + o[i]).rgb * w[i];
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
    vec3  sc  = sampleScaled(suv).rgb;
    float h   = fract(r * 3.0 + time * 2.0 + p);
    vec3  rb  = 0.5 + 0.5 * cos(6.2832 * (h + vec3(0.0, 0.333, 0.667)));
    return mix(col, mix(sc, rb, sp * 0.4), (1.0 - p) * 0.8);
}

// ─────────────────────────────────────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────────────────────────────────────
void main() {
    // 1. Barrel distortion (on original UV)
    vec2 uv = barrel(texcoord0);

    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // 2. Sync distortion and jitter
    uv = syncDistort(uv);
    if (jitter > 0.001)
        uv += vec2((noise(vec2(uv.y * resolution.y, floor(time * 60.0))) - 0.5)
                   * jitter * 0.003, 0.0);
    uv = clamp(uv, 0.001, 0.999);

    // 3. Pixel scaling + chromatic aberration
    // rbgShift also works on scaled UV so aberration aligns with pixels
    vec3 col;
    if (rbgShift > 0.001) {
        float sh = rbgShift * 0.004;
        col = vec3(
            sampleScaled(uv + vec2(sh,  0.0)).r,
            sampleScaled(uv).g,
            sampleScaled(uv - vec2(sh,  0.0)).b
        );
    } else {
        col = sampleScaled(uv).rgb;
    }

    // 4. Character smearing (horizontal blur)
    if (characterSmearing > 0.001) {
        // Smearing offset expressed in effective pixels
        float effX = mix(resolution.x, targetRes.x, pixelScale);
        vec2  px   = vec2(1.0 / effX, 0.0);
        col = mix(col,
                  (sampleScaled(uv - px).rgb + col
                   + sampleScaled(uv + px).rgb) / 3.0,
                  characterSmearing);
    }

    // 5. Bloom
    col += applyBloom(uv) * bloom;

    // 6. Ghosting
    if (ghostingIntensity > 0.001 && syncMode == 3)
        col += sampleScaled(uv + vec2(0.003, 0.0)).rgb * ghostingIntensity;

    // 7. Phosphor persistence (afterglow)
    if (phosphorPersistence > 0.001) {
        float effY  = mix(resolution.y, targetRes.y, pixelScale);
        vec3  prev  = sampleScaled(uv + vec2(0.0, 1.0 / effY)).rgb * 0.6;
        col = mix(col, col + prev * phosphorPersistence, 0.35);
    }

    // 8. Phosphor tint + color temperature + saturation
    float g = lum(col);
    col = mix(phosphorTint(vec3(g)), col, chromaColor);
    col = colorTemp(col);
    col = mix(vec3(lum(col)), col, 1.0 + saturationColor);

    // 9. Contrast / brightness
    col = clamp((col - 0.5) * (contrast + 0.5) + 0.5
                + (brightness - 0.5) * 0.4, 0.0, 1.0);

    // 10. Scanlines - align with effective resolution
    col *= scanlines(uv);

    // 11. Static noise
    if (staticNoise > 0.001)
        col += (noise(uv * resolution * 0.5
                + vec2(fract(time / 51.0), fract(time / 237.0)) * 500.0)
                - 0.5) * staticNoise * 0.15;

    // 12. Flicker
    if (flickering > 0.001) {
        float f = 0.9 + 0.1 * sin(time * 188.5)
                      + 0.05 * sin(time * 18.0)
                      + 0.02 * noise(vec2(time * 30.0, 0.0));
        col *= mix(1.0, f, flickering);
    }

    // 13. Vignette
    if (vignetteIntensity > 0.001) {
        vec2 c = uv - 0.5;
        col *= 1.0 - dot(c, c) * vignetteIntensity * 2.5;
    }

    // 14. Glass reflection
    if (ambientReflection > 0.001) {
        vec2  hs = uv - vec2(0.85, 0.08);
        float sp = exp(-dot(hs, hs) * 20.0);
        vec2  ed = min(uv, 1.0 - uv);
        col += (sp * 0.4 + (1.0 - smoothstep(0.0, 0.08, min(ed.x, ed.y))) * 0.3)
               * ambientReflection;
    }

    // 15. Burn-in
    if (burnIn > 0.001)
        col += col * (1.0 - length(uv - 0.5) * 0.8) * burnIn * 0.08;

    // 16. Glowing line
    if (glowingLine > 0.001) {
        float effY = mix(resolution.y, targetRes.y, pixelScale);
        col += vec3(exp(-abs(fract(uv.y * effY / 25.0) - 0.5) * 80.0)
                    * glowingLine * 0.06);
    }

    // 17. Animaties
    col = applyWarmup(col, uv);
    col = applyDegauss(col, uv);

    fragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
