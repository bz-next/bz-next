
out vec4 FragColor;
  
in vec2 interpolatedTextureCoordinates;

uniform sampler2D depthMap;

void main()
{             
    float depthValue = texture(depthMap, interpolatedTextureCoordinates).r;
    FragColor = vec4(vec3(depthValue), 1.0);
}