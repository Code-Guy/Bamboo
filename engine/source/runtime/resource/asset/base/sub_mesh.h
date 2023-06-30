#pragma once

#include "runtime/resource/asset/material.h"
#include "runtime/core/math/bounding_box.h"

namespace Bamboo
{
	class SubMesh : public IAssetRef
	{
	public:
		uint32_t m_index_offset;
		uint32_t m_index_count;
		uint32_t m_vertex_count;
		
		std::shared_ptr<Material> m_material;
		BoundingBox m_bounding_box;

	protected:
		virtual void bindRefs() override;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::base_class<IAssetRef>(this));
			ar(m_index_offset, m_index_count, m_vertex_count);
			ar(m_bounding_box);
		}
	};
}