#include "sky_light_component.h"
#include "runtime/resource/asset/asset_manager.h"
#include "runtime/function/render/pass/brdf_lut_pass.h"
#include "runtime/function/render/pass/filter_cube_pass.h"
#include "runtime/resource/asset/texture_2d.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::SkyLightComponent>("SkyLightComponent")
	 .property("texture_cube", &Bamboo::SkyLightComponent::m_texture_cube);
}

CEREAL_REGISTER_TYPE(Bamboo::SkyLightComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::LightComponent, Bamboo::SkyLightComponent)

namespace Bamboo
{

	SkyLightComponent::SkyLightComponent()
	{
		m_intensity = 0.5f;
		m_prefilter_mip_levels = 0;
	}

	SkyLightComponent::~SkyLightComponent()
	{
		m_irradiance_texture_sampler.destroy();
		m_prefilter_texture_sampler.destroy();
	}

	void SkyLightComponent::setTextureCube(std::shared_ptr<TextureCube>& texture_cube)
	{
		REF_ASSET(m_texture_cube, texture_cube)
		createIBLTextures();
	}

	void SkyLightComponent::bindRefs()
	{
		BIND_ASSET(m_texture_cube, TextureCube)
	}

	void SkyLightComponent::inflate()
	{
		// create brdf lut texture if not exist
		const auto& as = g_runtime_context.assetManager();
		if (!g_runtime_context.fileSystem()->exists(BRDF_TEXTURE_URL))
		{
			std::shared_ptr<BRDFLUTPass> brdf_pass = std::make_shared<BRDFLUTPass>();
			brdf_pass->init();
			brdf_pass->render();
			brdf_pass->destroy();
		}

		// create ibl textures
		createIBLTextures();
	}

	void SkyLightComponent::createIBLTextures()
	{
		std::shared_ptr<FilterCubePass> filter_cube_pass = std::make_shared<FilterCubePass>(m_texture_cube);
		filter_cube_pass->init();
		filter_cube_pass->render();
		filter_cube_pass->destroy();

		m_irradiance_texture_sampler.destroy();
		m_prefilter_texture_sampler.destroy();

		m_cube_mesh = filter_cube_pass->getCubeMesh();
		m_irradiance_texture_sampler = filter_cube_pass->getIrradianceTextureSampler();
		m_prefilter_texture_sampler = filter_cube_pass->getPrefilterTextureSampler();
		m_prefilter_mip_levels = filter_cube_pass->getPrefilterMipLevels();
		m_brdf_lut_texture_sampler = g_runtime_context.assetManager()->loadAsset<Texture2D>(BRDF_TEXTURE_URL)->m_image_view_sampler;
	}

}