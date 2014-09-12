varying vec2 vTexCoord;
uniform sampler2D uTexture;
uniform vec2 uResolution;
uniform vec2 uGazePoint;
uniform float uPeepholeRadius;

float dist() {
    vec2 coord = vTexCoord * uResolution;
    vec2 ref = uGazePoint * uResolution;
    return clamp(distance(coord, ref) / length(uResolution) / uPeepholeRadius, 0.0, 1.0);
}

void main(void)
{
    float blur = dist() / uPeepholeRadius / uResolution.x;
    vec3 sum = vec3(0.0);
    sum += texture2D(uTexture, vec2(vTexCoord.x - 4.0 * blur, vTexCoord.y)).rgb * 0.05;
    sum += texture2D(uTexture, vec2(vTexCoord.x - 3.0 * blur, vTexCoord.y)).rgb * 0.09;
    sum += texture2D(uTexture, vec2(vTexCoord.x - 2.0 * blur, vTexCoord.y)).rgb * 0.12;
    sum += texture2D(uTexture, vec2(vTexCoord.x - blur, vTexCoord.y)).rgb * 0.15;
    sum += texture2D(uTexture, vec2(vTexCoord.x, vTexCoord.y)).rgb * 0.16;
    sum += texture2D(uTexture, vec2(vTexCoord.x + blur, vTexCoord.y)).rgb * 0.15;
    sum += texture2D(uTexture, vec2(vTexCoord.x + 2.0 * blur, vTexCoord.y)).rgb * 0.12;
    sum += texture2D(uTexture, vec2(vTexCoord.x + 3.0 * blur, vTexCoord.y)).rgb * 0.09;
    sum += texture2D(uTexture, vec2(vTexCoord.x + 4.0 * blur, vTexCoord.y)).rgb * 0.05;
    gl_FragColor = vec4(sum, 1.0);
}
