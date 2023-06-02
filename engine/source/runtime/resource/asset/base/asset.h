#pragma once

#include <cereal/cereal.hpp>
#include <cereal/access.hpp>
#include <cereal/specialize.hpp>

namespace Bamboo
{
    using URL = std::string;

    struct RefAsset
    {
        URL url;
        std::shared_ptr<class Asset> asset;
    };

    class Asset
    {
    public:
        Asset() = default;
        virtual ~Asset() = default;

    protected:
        virtual void inflate() {}

    protected:
        std::vector<RefAsset> m_ref_assets;

    private:

    };
}