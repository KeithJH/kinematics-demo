#include "kinematics.h"
#include <cassert>
#include <cstdio>
#include <external/glad.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <vector>

namespace kinematics
{
ShaderSim::ShaderSim(const float width, const float height, const size_t numBodies)
	: Simulation(width, height)
{
	_shader = LoadShader(0, "shaders/fragment.glsl");
	glGenVertexArrays(1, &_vao);
	glBindVertexArray(_vao);

	glGenBuffers(1, &_vbo);
	SetNumBodies(numBodies);

	glVertexAttribPointer(static_cast<GLuint>(_shader.locs[SHADER_LOC_VERTEX_POSITION]), 2, GL_FLOAT, false,
	                      sizeof(Body), 0);

	glVertexAttribPointer(static_cast<GLuint>(_shader.locs[SHADER_LOC_VERTEX_COLOR]), 4, GL_UNSIGNED_BYTE, true,
	                      sizeof(Body), reinterpret_cast<void *>(offsetof(Body, color)));

	glEnableVertexAttribArray(static_cast<GLuint>(_shader.locs[SHADER_LOC_VERTEX_POSITION]));
	glEnableVertexAttribArray(static_cast<GLuint>(_shader.locs[SHADER_LOC_VERTEX_COLOR]));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	while (auto error = glGetError())
	{
		std::printf("error: %d\n", error);
	}
}

ShaderSim::~ShaderSim()
{
	glDeleteBuffers(1, &_vbo);
	glDeleteVertexArrays(1, &_vao);

	UnloadShader(_shader);
}

ShaderSim::ShaderSim(const float width, const float height, const Simulation &toCopy)
	: Simulation(width, height)
{
	for (const auto &body : toCopy.GetBodies())
	{
		_bodies.push_back(body);
	}
}

std::vector<Body> ShaderSim::GetBodies() const
{
	std::vector<Body> copy;
	copy.reserve(GetNumBodies());

	for (const auto &body : _bodies)
	{
		copy.emplace_back(body.x, body.y, body.horizontalSpeed, body.verticalSpeed, body.color);
	}

	return copy;
}

void ShaderSim::Update(const float deltaTime)
{
	for (auto &body : _bodies)
	{
		// Update position based on speed
		body.x += body.horizontalSpeed * deltaTime;
		body.y += body.verticalSpeed * deltaTime;

		// Bounce horizontally
		if (BounceCheck(body.x, body.horizontalSpeed, _width))
		{
			body.horizontalSpeed *= -1;
		}

		// Bounce vertically
		if (BounceCheck(body.y, body.verticalSpeed, _height))
		{
			body.verticalSpeed *= -1;
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(sizeof(Body) * _bodies.size()), _bodies.data());
}

void ShaderSim::Draw() const
{
	// `Draw()` should not be called when a window is not available
	assert(IsWindowReady());

	rlDrawRenderBatchActive();

	glUseProgram(_shader.id);

	Matrix modelViewProjection = rlGetMatrixProjection();

	glUniformMatrix4fv(_shader.locs[SHADER_LOC_MATRIX_MVP], 1, false, MatrixToFloat(modelViewProjection));

	glBindVertexArray(_vao);
	glPointSize(20);
	glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(_bodies.size()));
	glBindVertexArray(0);
	glUseProgram(0);

	/*
	while (auto error = glGetError())
	{
	    std::printf("error: %d\n", error);
	}
	*/
}

void ShaderSim::SetNumBodies(const size_t totalNumBodies)
{
	if (totalNumBodies > GetNumBodies())
	{
		_bodies.reserve(totalNumBodies);
		for (auto i = GetNumBodies(); i < totalNumBodies; i++)
		{
			AddRandomBody();
		}

		glBindBuffer(GL_ARRAY_BUFFER, _vbo);
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(Body) * _bodies.size()), _bodies.data(),
		             GL_STREAM_DRAW);
	}
	else
	{
		_bodies.resize(totalNumBodies);
	}
}

size_t ShaderSim::GetNumBodies() const { return _bodies.size(); }

void ShaderSim::AddRandomBody() { _bodies.push_back(GenerateRandomBody()); }
} // namespace kinematics
