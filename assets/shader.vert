#version 450

layout (std140, push_constant) uniform buf
{
    mat4 mvp;
	vec3 baseColor;
	mat4 invMvp;
	vec3 lightDir;
} ubuf;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outNormal;

void main()
{
	gl_Position = ubuf.mvp * vec4(position, 1.0f);
	outTexCoord = texCoord;
	outNormal = normal;
}
