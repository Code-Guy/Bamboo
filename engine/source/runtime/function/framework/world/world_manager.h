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

		bool loadWorld(const std::string& url);

	private:
		std::string m_active_world_url;
		std::shared_ptr<World> m_active_world;
	};
}