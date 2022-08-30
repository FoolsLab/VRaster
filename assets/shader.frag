//we will be using glsl version 4.5 syntax
#version 450
#extension GL_KHR_vulkan_glsl : enable

layout (std140, push_constant) uniform buf
{
    mat4 mvp;
	vec3 baseColor;
	mat4 invMvp;
	vec3 lightDir;
} ubuf;

layout(binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec3 normal;
layout (location = 0) out vec4 outFragColor;

void main()
{
	vec3 invLight  = normalize(ubuf.invMvp * vec4(ubuf.lightDir, 0.0)).xyz;
	float diffuse = clamp(dot(normal, invLight), 0.1, 1.0);
	vec4 sampledColor = texture(texSampler, texCoord);
	vec4 dColor = vec4(sampledColor.rgb * sampledColor.a + ubuf.baseColor * (1 - sampledColor.a), 1.0);
	// outFragColor = vec4(dColor.xyz * vec3(diffuse), 1.0);
	outFragColor = dColor;
}
