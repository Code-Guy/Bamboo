#pragma once

#include "engine/function/framework/entity/entity.h"
#include "engine/resource/asset/base/asset.h"

namespace Bamboo
{
	enum class EWorldMode
	{
		Edit, Play, Pause
	};

	class World : public Asset, public std::enable_shared_from_this<World>
	{
	public:
		~World();

		virtual void inflate() override;
		void tick(float delta_time);
		void step();

		const auto& getCameraEntity() { return m_camera_entity; }
		const auto& getEntities() const { return m_entities; }
		std::weak_ptr<Entity> getEntity(uint32_t id);
		std::weak_ptr<Entity> getEntity(const std::string& name);

		const std::shared_ptr<Entity>& createEntity(const std::string& name);
		bool removeEntity(uint32_t id);

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("entities", m_entities));
		}

		friend class WorldManager;
		World() = default;

		uint32_t m_next_entity_id = 0;
		std::weak_ptr<Entity> m_camera_entity;
		std::map<uint32_t, std::shared_ptr<Entity>> m_entities;

		bool is_stepping = false;
	};
}