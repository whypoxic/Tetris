#version 330 core

in vec2 TexCoord;
out vec4 fColor;

uniform sampler2D uTexture;
uniform float uAlpha;

void main()
{
    vec4 texColor = texture(uTexture, TexCoord);
    fColor = vec4(texColor.rgb, texColor.a * uAlpha);
}

