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
ShaderSim::ShaderSim(const float width, const float height, const size_t numBodies) : Simulation(width, height)
{
	_graphicsShader = LoadShader(0, "shaders/fragment.glsl");
	glGenVertexArrays(1, &_vao);
	glBindVertexArray(_vao);

	glGenBuffers(1, &_vbo);
	SetNumBodies(numBodies);

	glVertexAttribPointer(static_cast<GLuint>(_graphicsShader.locs[SHADER_LOC_VERTEX_POSITION]), 2, GL_FLOAT, false,
	                      sizeof(Body), 0);

	glVertexAttribPointer(static_cast<GLuint>(_graphicsShader.locs[SHADER_LOC_VERTEX_COLOR]), 4, GL_UNSIGNED_BYTE, true,
	                      sizeof(Body), reinterpret_cast<void *>(offsetof(Body, color)));

	glEnableVertexAttribArray(static_cast<GLuint>(_graphicsShader.locs[SHADER_LOC_VERTEX_POSITION]));
	glEnableVertexAttribArray(static_cast<GLuint>(_graphicsShader.locs[SHADER_LOC_VERTEX_COLOR]));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	char *computeShaderContent = LoadFileText("shaders/compute.glsl");
	_computeShader = rlCompileShader(computeShaderContent, RL_COMPUTE_SHADER);
	_computeProgram = rlLoadComputeShaderProgram(_computeShader);
	UnloadFileText(computeShaderContent);

	while (auto error = glGetError())
	{
		std::printf("error: %d\n", error);
	}
}

ShaderSim::~ShaderSim()
{
	glDeleteBuffers(1, &_vbo);
	glDeleteVertexArrays(1, &_vao);

	glDeleteShader(_computeShader);
	glDeleteProgram(_computeProgram);
	UnloadShader(_graphicsShader);
}

ShaderSim::ShaderSim(const float width, const float height, const Simulation &toCopy) : Simulation(width, height)
{
	//TODO: Needs updating like above
	for (const auto &body : toCopy.GetBodies())
	{
		_bodies.push_back(body);
	}
}

std::vector<Body> ShaderSim::GetBodies() const
{
	//TODO: doesn't copy from GPU memory
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
	constexpr unsigned WORKGROUP_SIZE = 1;
	constexpr unsigned WORKGROUP_LIMIT = 1 << 16;

	glUseProgram(_computeProgram);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _vbo);
	glUniform1f(1, deltaTime);
	glUniform1f(2, _width);
	glUniform1f(3, _height);

	const size_t numBodies = _bodies.size();
	glUniform1ui(4, static_cast<GLuint>(numBodies));

	const size_t numWorkGroups = (numBodies + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
	GLuint numWorkGroupsX = static_cast<GLuint>(numWorkGroups);
	GLuint numWorkGroupsY = 1;

	if (numWorkGroupsX > WORKGROUP_LIMIT)
	{
		numWorkGroupsX = WORKGROUP_LIMIT;
		numWorkGroupsY = static_cast<GLuint>((numWorkGroups + WORKGROUP_LIMIT - 1) / WORKGROUP_LIMIT);
	}

	glDispatchCompute(numWorkGroupsX, numWorkGroupsY, 1);
	glUseProgram(0);

	// TODO: Helps with timing?
	//glFinish();
}

void ShaderSim::Draw() const
{
	// `Draw()` should not be called when a window is not available
	assert(IsWindowReady());

	rlDrawRenderBatchActive();

	glUseProgram(_graphicsShader.id);

	Matrix modelViewProjection = rlGetMatrixProjection();

	glUniformMatrix4fv(_graphicsShader.locs[SHADER_LOC_MATRIX_MVP], 1, false, MatrixToFloat(modelViewProjection));

	glBindVertexArray(_vao);
	glPointSize(20);

	glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
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
		// TODO: we don't really need CPU buffer, if we have similar shader functionality
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
