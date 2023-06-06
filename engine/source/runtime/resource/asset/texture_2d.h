#pragma once

#include "runtime/resource/asset/base/texture.h"
#include "runtime/resource/asset/base/asset.h"

namespace Bamboo
{
	class Texture2D : public Texture, public Asset
	{
	public:
		Texture2D(const URL& url);
		virtual void inflate() override;

		std::vector<uint8_t> m_image_data;

	private:
		friend class cereal::access;
		template<class Archive>
		void archive(Archive& ar) const
		{
			ar(cereal::base_class<Texture>(this));
			ar(m_image_data.size());
		}

		template<class Archive>
		void save(Archive& ar) const
		{
			archive(ar);
		}

		template<class Archive>
		void load(Archive& ar)
		{
			archive(ar);

			inflate();
		}
	};
}