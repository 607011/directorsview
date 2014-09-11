// Fade-to-black fragment shader.
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
    float dist = 1.0 - distance(uGazePoint, vTexCoord);
    vec3 rgb = texture2D(uTexture, vTexCoord.st).rgb;
    gl_FragColor = vec4(rgb * clamp(2.0 * dist, 0.1, 1.0), 1.0);
}
