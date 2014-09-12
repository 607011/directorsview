// Fade-to-black-from-peephole fragment shader.
//
// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

varying vec2 vTexCoord;
uniform sampler2D uTexture;
uniform vec2 uResolution;
uniform vec2 uGazePoint;
uniform float uPeepholeRadius;

void main(void)
{
    vec2 coord = vTexCoord * uResolution;
    vec2 ref = uGazePoint * uResolution;
    float dist = clamp(distance(coord, ref) / length(uResolution) / uPeepholeRadius, 0.1, 1.0);
    gl_FragColor = vec4(texture2D(uTexture, vTexCoord.st).rgb * (1.0 - dist), 1.0);
}
