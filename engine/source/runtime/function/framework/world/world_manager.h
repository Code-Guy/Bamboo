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
		bool createWorld(const URL& template_url, const URL& save_as_url);
		bool saveWorld();
		bool saveAsWorld(const URL& url);

		const std::shared_ptr<World>& getCurrentWorld() { return m_current_world; }
		std::string getCurrentWorldName();

		std::shared_ptr<class CameraComponent> getCameraComponent();

	private:
		void scriptWorld();

		std::shared_ptr<World> m_current_world;
	};
}