#pragma once

#include "runtime/resource/asset/base/bone.h"

namespace Bamboo
{
	class Skeleton : public Asset
	{
	public:
		std::string m_name;
		std::vector<Bone> m_bones;
		uint8_t m_root_bone_index;

		std::map<std::string, uint8_t> m_name_index;

		virtual void inflate() override;

		bool has_bone(const std::string& name);
		Bone* get_bone(const std::string& name);
		void update();

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(m_name, m_bones, m_root_bone_index);
		}
	};
}