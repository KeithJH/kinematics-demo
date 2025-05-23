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

// Rough helper to print OpenGL erros.
// Should only be used for testing, or made more robust.
void printGlErrors()
{
	while (auto error = glGetError())
	{
		std::printf("error: %d\n", error);
	}
}

ShaderSim::ShaderSim(const float width, const float height, const size_t numBodies)
	: Simulation(width, height), _numBodies(0), _maxBodies(0)
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

	printGlErrors();
}

ShaderSim::~ShaderSim()
{
	glDeleteBuffers(1, &_vbo);
	glDeleteVertexArrays(1, &_vao);

	glDeleteShader(_computeShader);
	glDeleteProgram(_computeProgram);
	UnloadShader(_graphicsShader);
}

std::vector<Body> ShaderSim::GetBodies() const
{
	std::vector<Body> copy(GetNumBodies());

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _vbo);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, static_cast<GLsizeiptr>(sizeof(Body) * copy.size()), copy.data());

	return copy;
}

void ShaderSim::Update(const float deltaTime)
{
	constexpr unsigned WORKGROUP_SIZE = 1024;
	constexpr unsigned WORKGROUP_LIMIT = 1 << 16;

	glUseProgram(_computeProgram);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _vbo);
	glUniform1f(1, deltaTime);
	glUniform1f(2, _width);
	glUniform1f(3, _height);

	const size_t numBodies = _numBodies;
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
	glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(_numBodies));

	glBindVertexArray(0);
	glUseProgram(0);

	// printGlErrors();
}

void ShaderSim::SetNumBodies(const size_t totalNumBodies)
{
	if (totalNumBodies > _maxBodies)
	{
		// TODO: we don't really need CPU buffer, if we have similar shader functionality
		std::vector<Body> bodies = GetBodies();
		bodies.reserve(totalNumBodies);

		for (auto i = GetNumBodies(); i < totalNumBodies; i++)
		{
			bodies.push_back(GenerateRandomBody());
		}

		glBindBuffer(GL_ARRAY_BUFFER, _vbo);
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(Body) * totalNumBodies), bodies.data(),
		             GL_DYNAMIC_COPY);

		_maxBodies = totalNumBodies;
	}

	_numBodies = totalNumBodies;
}

size_t ShaderSim::GetNumBodies() const { return _numBodies; }

void ShaderSim::AddRandomBody() {}
} // namespace kinematics
