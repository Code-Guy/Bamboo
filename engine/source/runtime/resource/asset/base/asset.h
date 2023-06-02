#pragma once

#include <memory>
#include <string>
#include <vector>

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

        virtual void inflate() {}

    protected:
        std::vector<RefAsset> m_ref_assets;

    private:

    };
}