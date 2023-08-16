#pragma once

#include "world.h"

namespace Bamboo
{
	class WorldManager
	{
	public:
		void init();
		void destroy();
		void tick(float delta_time);

		bool loadWorld(const URL& url);
		std::shared_ptr<World> createWorld(const URL& url);
		const std::shared_ptr<World>& getCurrentWorld() { return m_current_world; }
		const std::string& getCurrentWorldName() { return m_current_world_name; }

		std::shared_ptr<class CameraComponent> getCameraComponent();

	private:
		void scriptWorld();

		std::string m_current_world_url;
		std::string m_current_world_name;
		std::shared_ptr<World> m_current_world;
	};
}