#pragma once

#include "runtime/function/framework/entity/entity.h"
#include "runtime/resource/asset/base/asset.h"
#include <map>

namespace Bamboo
{
	class World : public Asset
	{
	public:
		void tick(float delta_time);
		void inflate();

		const auto& getCameraEntity() { return m_camera_entity; }
		const auto& getEntities() const { return m_entites; }
		std::shared_ptr<Entity> getEntity(uint32_t id);

		std::shared_ptr<Entity> createEntity(const std::string& name);
		bool removeEntity(uint32_t id);

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("entity_counter", m_entity_counter));
			ar(cereal::make_nvp("camera_entity", m_camera_entity));
			ar(cereal::make_nvp("entites", m_entites));
		}

		friend class WorldManager;
		World() = default;

		uint32_t m_entity_counter = 0;
		std::shared_ptr<Entity> m_camera_entity;
		std::map<uint32_t, std::shared_ptr<Entity>> m_entites;
	};
}