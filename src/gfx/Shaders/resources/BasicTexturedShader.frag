precision highp float;
precision highp int;

uniform vec3 color
    #ifndef GL_ES
    = vec3(1.0, 1.0, 1.0)
    #endif
    ;

uniform lowp sampler2D textureData;

in vec2 interpolatedTextureCoordinates;

out vec4 fragmentColor;

void main() {
    fragmentColor.rgb = color*texture(textureData, interpolatedTextureCoordinates).rgb;
    fragmentColor.a = 1.0;
}