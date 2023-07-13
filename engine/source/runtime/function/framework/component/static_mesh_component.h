#pragma once

#include "component.h"
#include "runtime/resource/asset/static_mesh.h"

namespace Bamboo
{
	class StaticMeshComponent : public Component, public IAssetRef
	{
	public:
		void setStaticMesh(std::shared_ptr<StaticMesh>& static_mesh);
		std::shared_ptr<StaticMesh> getStaticMesh() { return m_static_mesh; }

	private:
		REGISTER_COMPONENT

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("component", cereal::base_class<Component>(this)));
			ar(cereal::make_nvp("asset_ref", cereal::base_class<IAssetRef>(this)));
			bindRefs();
		}

		virtual void bindRefs() override;

		std::shared_ptr<StaticMesh> m_static_mesh;
	};
}