precision highp float;
precision highp int;

layout (location = 0) in vec3 position;

uniform mat4 projectionMatrix;
uniform mat4 transformationMatrix;

void main()
{
    gl_Position = projectionMatrix * transformationMatrix * vec4(position, 1.0);
}