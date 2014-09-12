// Pixelize fragment shader.
//
// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

varying vec2 vTexCoord;
uniform sampler2D uTexture;
uniform vec2 uResolution;
uniform vec2 uGazePoint;
uniform float uPeepholeRadius;

const float maxPixelSize = 20.0;

void main(void)
{
    vec2 coord = vTexCoord * uResolution;
    vec2 ref = uGazePoint * uResolution;
    float dist = clamp(distance(coord, ref) / length(uResolution) / uPeepholeRadius, 0.0, 1.0);
    vec2 pixelWidth = dist * maxPixelSize / uResolution;
    coord = floor(vTexCoord / pixelWidth) * pixelWidth + pixelWidth / 2.0;
    gl_FragColor = texture2D(uTexture, coord);
}
