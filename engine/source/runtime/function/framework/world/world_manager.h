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
		std::shared_ptr<World> getCurrentWorld() { return m_current_world; }

	private:
		std::string m_current_world_url;
		std::shared_ptr<World> m_current_world;
	};
}