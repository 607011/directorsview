// Default fragment shader.
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
    gl_FragColor = texture2D(uTexture, vTexCoord);
}
