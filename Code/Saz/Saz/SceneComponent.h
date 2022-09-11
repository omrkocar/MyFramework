#pragma once

#include "Core/String.h"
#include "glm/glm.hpp"

namespace component
{
	struct LoadedSceneComponent
	{
		String Name = {};
		String Path = {};
		glm::vec2 SceneSize;
		bool IsFocused;
		bool IsHovered;
	};

	struct NewSceneRequestOneFrameComponent
	{
		String Path = {};
	};

	struct LoadSceneRequestOneFrameComponent
	{
		String Path = {};
	};

	struct SaveSceneRequestOneFrameComponent
	{
		String Path = {};
	};

	struct SceneEntityComponent
	{
		String ScenePath{};
	};
}