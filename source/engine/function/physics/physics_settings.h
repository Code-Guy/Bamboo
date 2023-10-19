#pragma once

#include <Jolt/Physics/PhysicsSettings.h>

namespace Bamboo
{
	struct PhysicsSettings : public JPH::PhysicsSettings
	{
		float m_update_delta_time = 0.0167f;

		int m_max_bodies = 1024;
		int m_max_body_mutex = 0;
		int m_max_body_pairs = 1024;
		int m_max_contact_constraints = 1024;
		int m_temp_allocator_size = 1024 * 1024 * 10;
	};
}