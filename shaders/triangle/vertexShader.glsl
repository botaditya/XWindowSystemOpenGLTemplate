#version 460 core

in vec4 aPosition;
in vec4 aColor;
out vec4 oColor;

void main(void)
{
    gl_Position = aPosition;
    oColor = aColor;
};
