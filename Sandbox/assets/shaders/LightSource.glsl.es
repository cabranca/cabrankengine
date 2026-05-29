#type vertex
#version 300 es
precision highp float;

layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

#type fragment
#version 300 es
precision mediump float;

out vec4 FragColor;

uniform vec3 debugColor;

void main()
{
    FragColor = vec4(debugColor, 1.0);
}
