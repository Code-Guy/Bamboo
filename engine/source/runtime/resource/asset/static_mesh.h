#pragma once

#include "runtime/resource/asset/base/mesh.h"
#include "runtime/resource/asset/base/asset.h"

namespace Bamboo
{
	class StaticMesh : public Mesh, public Asset
	{
	public:
		virtual void inflate() override;

		std::vector<StaticVertex> m_vertices;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::base_class<Mesh>(this));
			ar(m_vertices);
		}
	};
}