#version 430 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

struct Body
{
	float x;
	float y;
	float horizontalSpeed;
	float verticalSpeed;
	uint color;
};

layout (binding = 0) restrict buffer vbo
{
	Body body[];
};

layout (location = 1) uniform float deltaTime;
layout (location = 2) uniform float horizontalBounds;
layout (location = 3) uniform float verticalBounds;
layout (location = 4) uniform uint numBodies;

const float BODY_RADIUS = 10.0f;
bool BounceCheck(const float position, const float speed, const float bounds)
{
	return (position - BODY_RADIUS < 0 && speed < 0) || (position + BODY_RADIUS > bounds && speed > 0);
}

void main()
{
	uint i = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x;
	if (i < numBodies)
	{
		body[i].x += deltaTime * body[i].horizontalSpeed;
		body[i].y += deltaTime * body[i].verticalSpeed;

		if (BounceCheck(body[i].x, body[i].horizontalSpeed, horizontalBounds))
		{
			body[i].horizontalSpeed *= -1;
		}

		if (BounceCheck(body[i].y, body[i].verticalSpeed, verticalBounds))
		{
			body[i].verticalSpeed *= -1;
		}
	}
}
