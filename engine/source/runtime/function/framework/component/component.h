#pragma once

#include <memory>
#include <string>
#include <chrono>

#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/json.hpp>

#include "runtime/resource/serialization/serialization.h"

namespace Bamboo
{
	class ITickable
	{
	public:
		void setTickEnabled(bool tick_enabled) { m_tick_enabled = tick_enabled; }
		void setTickInterval(float tick_interval) { m_tick_interval = tick_interval; }

		void tickable(float delta_time);
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

		void attach(std::weak_ptr<Entity>& parent);
		void dettach();
		std::weak_ptr<Entity>& getParent() { return m_parent; }

		virtual void inflate() {}

	protected:
		std::weak_ptr<Entity> m_parent;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("tickable", cereal::base_class<ITickable>(this)));
		}
	};
}