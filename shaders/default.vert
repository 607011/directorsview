// Default vertex shader.
//
// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.


attribute vec4 aVertex;
attribute vec2 aTexCoord;
varying vec2 vTexCoord;

void main(void) {
    vTexCoord = aTexCoord;
    gl_Position = aVertex;
}
