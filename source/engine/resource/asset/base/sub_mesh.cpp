#include "sub_mesh.h"
#include "engine/function/global/engine_context.h"
#include "engine/resource/asset/asset_manager.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::SubMesh>("SubMesh")
	 .property("material", &Bamboo::SubMesh::m_material);
}

namespace Bamboo
{

	void SubMesh::bindRefs()
	{
		BIND_ASSET(m_material, Material)
	}

}