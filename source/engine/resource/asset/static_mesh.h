#pragma once

#include "engine/resource/asset/base/mesh.h"
#include "engine/resource/asset/base/asset.h"

namespace Bamboo
{
	class StaticMesh : public Mesh, public Asset
	{
	public:
		virtual void inflate() override;

		std::vector<StaticVertex> m_vertices;

	protected:
		virtual void calcBoundingBox() override;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("mesh", cereal::base_class<Mesh>(this)));
			ar(cereal::make_nvp("vertices", m_vertices));
		}
	};
}