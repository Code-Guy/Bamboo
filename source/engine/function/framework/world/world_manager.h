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

		void openWorld(const URL& url);
		void createWorld(const URL& template_url, const URL& save_as_url);
		bool saveWorld();
		bool saveAsWorld(const URL& url);

		const std::shared_ptr<World>& getCurrentWorld() { return m_current_world; }
		std::string getCurrentWorldName();

		std::weak_ptr<class CameraComponent> getCameraComponent();

		void setWorldMode(EWorldMode world_mode);
		EWorldMode getWorldMode() { return m_world_mode; }

	private:
		bool loadWorld(const URL& url);
		void scriptWorld();

		std::shared_ptr<World> m_current_world;

		URL m_open_world_url;
		URL m_template_url, m_save_as_url;

		EWorldMode m_world_mode;
	};
}