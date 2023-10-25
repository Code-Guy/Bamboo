#pragma once

#include <memory>
#include <map>
#include <glm/glm.hpp>

namespace JPH
{
	class PhysicsSystem;
	class JobSystemThreadPool;
	class TempAllocatorImpl;
	class BodyInterface;
	class ObjectLayerPairFilter;
	class BroadPhaseLayerInterface;
	class ObjectVsBroadPhaseLayerFilter;
	class ContactListener;
	class BodyActivationListener;
}

namespace Bamboo
{
	class PhysicsSystem
	{
	public:
		PhysicsSystem();
		~PhysicsSystem();

		void init();
		void destroy();

	private:
		void tick();
		void collectRigidbodies();
		void clearRigidbodies();

		std::unique_ptr<class PhysicsSettings> m_physics_settings;

		std::unique_ptr<class JPH::PhysicsSystem> m_physics_system;
		std::unique_ptr<class JPH::TempAllocatorImpl> m_temp_allocator;
		std::unique_ptr<class JPH::JobSystemThreadPool> m_job_system;
		class JPH::BodyInterface* m_body_interface;

		std::unique_ptr<class JPH::ObjectLayerPairFilter> m_object_layer_pair_filter;
		std::unique_ptr<class JPH::BroadPhaseLayerInterface> m_broad_phase_layer_interface;
		std::unique_ptr<class JPH::ObjectVsBroadPhaseLayerFilter> m_objec_vs_broad_phase_layer_filter;
		std::unique_ptr<class JPH::ContactListener> m_contact_listenser;
		std::unique_ptr<class JPH::BodyActivationListener> m_body_activation_listener;

		uint32_t m_tick_timer_handle;
		std::map<uint32_t, std::shared_ptr<class TransformComponent>> m_body_transforms;
	};
}