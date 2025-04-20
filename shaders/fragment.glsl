#version 430 core

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Output fragment color
out vec4 finalColor;

void main()
{
	vec2 uv = gl_PointCoord.xy * 2.f - 1.f;
	finalColor = vec4(fragColor.rgb, 1);

	if (length(uv) > 1)
		discard;
}
