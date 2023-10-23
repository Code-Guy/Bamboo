#pragma once

#include "collider_component.h"
#include "engine/resource/asset/static_mesh.h"

namespace Bamboo
{
	class MeshColliderComponent : public ColliderComponent, public IAssetRef
	{
	public:
		MeshColliderComponent();

		void setStaticMesh(std::shared_ptr<StaticMesh>& static_mesh);
		std::shared_ptr<StaticMesh> getStaticMesh() { return m_static_mesh; }

	private:
		REGISTER_REFLECTION(ColliderComponent)

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("component", cereal::base_class<ColliderComponent>(this)));
		}

		virtual void bindRefs() override;

		std::shared_ptr<StaticMesh> m_static_mesh;
	};
}