#include "physics_system.h"
#include "engine/core/base/macro.h"
#include "engine/platform/timer/timer.h"
#include "engine/function/framework/component/rigidbody_component.h"
#include "engine/function/framework/component/box_collider_component.h"
#include "engine/function/framework/component/sphere_collider_component.h"
#include "engine/function/framework/component/capsule_collider_component.h"
#include "engine/function/framework/component/cylinder_collider_component.h"
#include "engine/function/framework/component/mesh_collider_component.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include "physics_settings.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <cstdarg>

namespace Bamboo
{
	struct ObjectLayers
	{
		static constexpr JPH::ObjectLayer NonMoving = 0;
		static constexpr JPH::ObjectLayer Moving = 1;
		static constexpr uint32_t LayerNum = 2;
	};

	namespace BroadPhaseLayers
	{
		static constexpr JPH::BroadPhaseLayer NonMoving(0);
		static constexpr JPH::BroadPhaseLayer Moving(1);
		static constexpr uint32_t LayerNum = 2;
	};

	// jolt traces callback
	static void joltTraceImpl(const char* fmt, ...)
	{
		// Format the message
		va_list list;
		va_start(list, fmt);
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), fmt, list);
		va_end(list);

		LOG_INFO("jolt trace: {}", buffer);
	}

	// jolt assert callback
	static bool joltAssertImpl(const char* expression, const char* message, const char* file, unsigned int line)
	{
		LOG_INFO("jolt assert: {}:{}:({}):{}", file, line, expression, message != nullptr ? message : "");
		return true;
	};

	/// determines if two object layers can collide
	class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer lhs, JPH::ObjectLayer rhs) const override
		{
			switch (lhs)
			{
			case ObjectLayers::NonMoving:
			{
				return rhs == ObjectLayers::Moving;
			}
			case ObjectLayers::Moving:
			{
				return true;
			}
			default:
			{
				return false;
			}
			}
		}
	};

	// defines a mapping between object and broadphase layers
	class BroadPhaseLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
	{
	public:
		BroadPhaseLayerInterfaceImpl()
		{
			// Create a mapping table from object to broad phase layer
			m_object_to_broad_phase[ObjectLayers::NonMoving] = JPH::BroadPhaseLayer(BroadPhaseLayers::NonMoving);
			m_object_to_broad_phase[ObjectLayers::Moving] = JPH::BroadPhaseLayer(BroadPhaseLayers::Moving);
		}

		virtual uint32_t GetNumBroadPhaseLayers() const override
		{
			return BroadPhaseLayers::LayerNum;
		}

		virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
		{
			JPH_ASSERT(layer < ObjectLayers::LayerNum);
			return m_object_to_broad_phase[layer];
		}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
		{
			switch ((JPH::BroadPhaseLayer::Type)layer)
			{
			case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NonMoving:
			{
				return "NonMoving";
			}
			case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::Moving:
			{
				return "Moving";
			}
			default:
			{
				return "Invalid";
			}
			}
		}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

	private:
		JPH::BroadPhaseLayer m_object_to_broad_phase[ObjectLayers::LayerNum];
	};

	// determines if an object layer can collide with a broadphase layer
	class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer lhs, JPH::BroadPhaseLayer rhs) const override
		{
			switch (lhs)
			{
			case ObjectLayers::NonMoving:
			{
				return rhs == BroadPhaseLayers::Moving;
			}
			case ObjectLayers::Moving:
			{
				return true;
			}
			default:
			{
				return false;
			}
			}
		}
	};

	// deal with contact
	class ContactListenerImpl : public JPH::ContactListener
	{
	public:
		virtual JPH::ValidateResult	OnContactValidate(const JPH::Body& body1, const JPH::Body& body2, JPH::RVec3Arg base_offset, const JPH::CollideShapeResult& collision_result) override
		{
			LOG_INFO("jolt on contact validate");

			// allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
			return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
		}

		virtual void OnContactAdded(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& settings) override
		{
			LOG_INFO("jolt on contact added");
		}

		virtual void OnContactPersisted(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& settings) override
		{
			LOG_INFO("jolt on contact persisted");
		}

		virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override
		{
			LOG_INFO("jolt on contact removed");
		}
	};

	// deal with body activation
	class BodyActivationListenerImpl : public JPH::BodyActivationListener
	{
	public:
		virtual void OnBodyActivated(const JPH::BodyID& body_id, uint64_t body_user_data) override
		{
			LOG_INFO("jolt on body activated");
		}

		virtual void OnBodyDeactivated(const JPH::BodyID& body_id, uint64_t body_user_data) override
		{
			LOG_INFO("jolt on body deactivated");
		}
	};

	PhysicsSystem::PhysicsSystem() = default;
	PhysicsSystem::~PhysicsSystem() = default;

	void PhysicsSystem::init()
	{
		// register allocation hook
		JPH::RegisterDefaultAllocator();

		// register callbacks
		JPH::Trace = joltTraceImpl;
		JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = joltAssertImpl;)

			// create factory
			JPH::Factory::sInstance = new JPH::Factory();

		// register all jolt types
		JPH::RegisterTypes();

		// init temp allocator
		m_physics_settings = std::make_unique<PhysicsSettings>();
		m_temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(m_physics_settings->m_temp_allocator_size);

		// init job system
		m_job_system = std::make_unique<JPH::JobSystemThreadPool>();
		m_job_system->Init(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

		// init layers
		m_object_layer_pair_filter = std::make_unique<ObjectLayerPairFilterImpl>();
		m_broad_phase_layer_interface = std::make_unique<BroadPhaseLayerInterfaceImpl>();
		m_objec_vs_broad_phase_layer_filter = std::make_unique<ObjectVsBroadPhaseLayerFilterImpl>();

		// init physics system
		m_physics_system = std::make_unique<JPH::PhysicsSystem>();
		m_physics_system->Init(m_physics_settings->m_max_bodies, m_physics_settings->m_max_body_mutex,
			m_physics_settings->m_max_body_pairs, m_physics_settings->m_max_contact_constraints, *m_broad_phase_layer_interface.get(),
			*m_objec_vs_broad_phase_layer_filter.get(), *m_object_layer_pair_filter.get());

		// set listeners
		m_contact_listenser = std::make_unique<ContactListenerImpl>();
		m_body_activation_listener = std::make_unique<BodyActivationListenerImpl>();
		m_physics_system->SetContactListener(m_contact_listenser.get());
		m_physics_system->SetBodyActivationListener(m_body_activation_listener.get());

		// get body interface
		m_body_interface = &m_physics_system->GetBodyInterface();

		// start ticking physics system
		m_tick_timer_handle = g_engine.timerManager()->addTimer(m_physics_settings->m_update_delta_time, [this]() { tick(); }, true);
	}

	void PhysicsSystem::destroy()
	{
		// unregister all jolt types
		JPH::UnregisterTypes();

		// destroy the factory
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;

		// stop ticking physics system
		g_engine.timerManager()->removeTimer(m_tick_timer_handle);
	}

	JPH::Vec3 glmVec3ToJPHVec3(const glm::vec3& v)
	{
		return JPH::Vec3(v.x, v.y, v.z);
	}

	JPH::Quat glmQuatToJPHQuat(const glm::quat& q)
	{
		return JPH::Quat(q.x, q.y, q.z, q.w);
	}

	JPH::ObjectLayer motionTypeToObjectLayer(EMotionType motion_type)
	{
		if (motion_type == EMotionType::Static)
		{
			return ObjectLayers::NonMoving;
		}
		return ObjectLayers::Moving;
	}

	JPH::ShapeSettings* makeShapeSettingsFromCollider(const glm::vec3& scale, const std::shared_ptr<class ColliderComponent>& collider_component)
	{
		EColliderType collider_type = collider_component->m_type;
		JPH::ShapeSettings* shape_settings = nullptr;
		float max_scale = std::max(std::max(scale.x, scale.y), scale.z);

		switch (collider_type)
		{
		case EColliderType::Box:
		{
			std::shared_ptr<BoxColliderComponent> collider = std::static_pointer_cast<BoxColliderComponent>(collider_component);
			shape_settings = new JPH::BoxShapeSettings(glmVec3ToJPHVec3(collider->m_size * scale));
		}
		break;
		case EColliderType::Sphere:
		{
			std::shared_ptr<SphereColliderComponent> collider = std::static_pointer_cast<SphereColliderComponent>(collider_component);
			shape_settings = new JPH::SphereShapeSettings(collider->m_radius * max_scale);
		}
		break;
		case EColliderType::Capsule:
		{
			std::shared_ptr<CapsuleColliderComponent> collider = std::static_pointer_cast<CapsuleColliderComponent>(collider_component);
			shape_settings = new JPH::CapsuleShapeSettings(collider->m_height * 0.5f * scale.z, collider->m_radius * std::max(scale.x, scale.y));
		}
		break;
		case EColliderType::Cylinder:
		{
			std::shared_ptr<CylinderColliderComponent> collider = std::static_pointer_cast<CylinderColliderComponent>(collider_component);
			shape_settings = new JPH::CylinderShapeSettings(collider->m_height * 0.5f * scale.z, collider->m_radius * std::max(scale.x, scale.y));
		}
		break;
		case EColliderType::Mesh:
		{

		}
		break;
		default:
			break;
		}

		return shape_settings;
	}

	uint32_t PhysicsSystem::addRigidbody(const glm::mat4& global_matrix, class RigidbodyComponent* rigidbody_component,
		const std::vector<std::shared_ptr<class ColliderComponent>>& collider_components)
	{
		glm::quat rotation;
		glm::vec3 scale, translation, skew;
		glm::vec4 perspective;
		glm::decompose(global_matrix, scale, rotation, translation, skew, perspective);
		std::vector<JPH::ShapeSettings*> shape_settings_list;
		for (const auto& collider_component : collider_components)
		{
			shape_settings_list.push_back(makeShapeSettingsFromCollider(scale, collider_component));
		}

		JPH::ShapeSettings* shape_settings;
		if (shape_settings_list.size() == 1)
		{
			shape_settings = shape_settings_list.front();
		}
		else
		{
			JPH::StaticCompoundShapeSettings* compound_shape_settings = new JPH::StaticCompoundShapeSettings;
			for (JPH::ShapeSettings* sub_shape_settings : shape_settings_list)
			{
				compound_shape_settings->AddShape(JPH::Vec3::sZero(), JPH::Quat::sIdentity(), sub_shape_settings);
			}
			shape_settings = compound_shape_settings;
		}

		EMotionType motion_type = rigidbody_component->m_motion_type;
		JPH::BodyCreationSettings body_creation_settings(shape_settings, glmVec3ToJPHVec3(translation), 
			glmQuatToJPHQuat(rotation), (JPH::EMotionType)motion_type, motionTypeToObjectLayer(motion_type));
		JPH::BodyID body_id = m_body_interface->CreateAndAddBody(body_creation_settings, 
			motion_type == EMotionType::Static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);

		ASSERT(!body_id.IsInvalid(), "jolt run out of bodies");
		return body_id.GetIndexAndSequenceNumber();
	}

	void PhysicsSystem::removeRigidbody(uint32_t body_id)
	{
		m_pending_remove_body_ids.push_back(body_id);
	}

	void PhysicsSystem::tick()
	{
		static StopWatch stop_watch;
		float delta_time = stop_watch.stop();

		if (delta_time > 0.001f && g_engine.isPlaying())
		{
			// update bodies
			int collision_step = (int)std::ceilf(delta_time / m_physics_settings->m_update_delta_time);
			m_physics_system->Update(delta_time, collision_step, m_temp_allocator.get(), m_job_system.get());

			// remove bodies
			for (uint32_t remove_body_id : m_pending_remove_body_ids)
			{
				LOG_INFO("remove body {}", remove_body_id);
				m_body_interface->RemoveBody(JPH::BodyID(remove_body_id));
				m_body_interface->DestroyBody(JPH::BodyID(remove_body_id));
			}
			m_pending_remove_body_ids.clear();
		}

		stop_watch.start();
	}

}