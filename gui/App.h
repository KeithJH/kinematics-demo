#pragma once
#include "../kinematics/kinematics.h"

class App
{
  public:
	App(const float width, const float height, const size_t initialNumBodies);

	void Update();

	/// Top level draw function for the entire scene.
	/// Includes: Background, FPS counter, statistics and `Simulation` contents.
	void DrawFrame() const;

  private:
	bool _renderBodies = true, _renderStats = false;
	bool _updateBodies = true;
	kinematics::Simulation _simulation;

	float _frameTimeSeconds;
	long _updateMicroseconds;

  private:
	/// Update the simulation according to user input.
	/// Includes: Toggle for rendering bodies, toggle for updating bodies, setting number of bodies
	void HandleInput();
};
