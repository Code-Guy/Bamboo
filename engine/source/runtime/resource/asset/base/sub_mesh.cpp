#include "sub_mesh.h"
#include "runtime/function/global/runtime_context.h"
#include "runtime/resource/asset/asset_manager.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::SubMesh>("SubMesh")
	 .property("m_material", &Bamboo::SubMesh::m_material);
}

namespace Bamboo
{

	void SubMesh::bindRefs()
	{
		BIND_ASSET(m_material, Material)
	}

}