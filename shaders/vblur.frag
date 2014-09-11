// texture coordinate (0;0)..(1;1)
varying vec2 vTexCoord;
// texture
uniform sampler2D uTexture;
// elapsed time since program start in seconds (with fractions)
uniform float uT;
// width by height of texture
uniform vec2 uResolution;
// mouse position within range of uResolution
uniform vec2 uGazePoint;

void main(void)
{
  gl_FragColor = sampler2D(uTexture, vTexCoord);
}
