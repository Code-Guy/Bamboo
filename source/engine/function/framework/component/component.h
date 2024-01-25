#pragma once

#include <memory>
#include <string>
#include <chrono>

#include <rttr/registration>
#include <rttr/registration_friend.h>
#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/json.hpp>

#include "engine/resource/serialization/serialization.h"

namespace Bamboo
{
	class ITickable
	{
	public:
		void setTickEnabled(bool tick_enabled) { m_tick_enabled = tick_enabled; }
		void setTickInterval(float tick_interval) { m_tick_interval = tick_interval; }

		void tickable(float delta_time);

	protected:
		virtual void tick(float delta_time) {}

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("tick_enabled", m_tick_enabled));
			ar(cereal::make_nvp("tick_interval", m_tick_interval));
		}

		bool m_tick_enabled = false;
		float m_tick_interval = 0.0f;
		float m_tick_timer = 0.0f;
		std::chrono::time_point<std::chrono::steady_clock> m_last_tick_time = std::chrono::steady_clock::now();
	};

	class Entity;
	class Component : public ITickable
	{
	public:
		Component() = default;
		virtual ~Component() = default;

		void attach(const std::weak_ptr<Entity>& parent);
		void detach();
		std::weak_ptr<Entity>& getParent() { return m_parent; }
		const std::string& getTypeName() { return m_type_name; }
		void setTypeName(const std::string& type_name) { m_type_name = type_name; }

	protected:
		virtual void inflate() {}
		virtual void beginPlay() {}
		virtual void endPlay() {}

		std::weak_ptr<Entity> m_parent;
		std::string m_type_name;

	private:
		friend Entity;

		RTTR_ENABLE()
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("tickable", cereal::base_class<ITickable>(this)));
		}
	};
}

#define REGISTER_REFLECTION(parent_class) \
	RTTR_REGISTRATION_FRIEND \
	RTTR_ENABLE(Bamboo::parent_class) \
	friend class cereal::access;

#define POLYMORPHIC_DECLARATION virtual void inflate() override;
#define POLYMORPHIC_DEFINITION(class_name) void class_name::inflate() {}
	