#include "render_system.h"
#include "runtime/core/base/macro.h"
#include "runtime/core/event/event_system.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/resource/asset/asset_manager.h"

#include "runtime/core/vulkan/vulkan_rhi.h"
#include "runtime/function/render/pass/main_pass.h"
#include "runtime/function/render/pass/ui_pass.h"

#include "runtime/function/framework/component/camera_component.h"
#include "runtime/function/framework/component/transform_component.h"
#include "runtime/function/framework/component/static_mesh_component.h"
#include "runtime/function/framework/component/skeletal_mesh_component.h"
#include "runtime/function/framework/component/sky_light_component.h"

namespace Bamboo
{

	void RenderSystem::init()
	{		
		m_render_passes[ERenderPassType::Main] = std::make_shared<MainPass>();
		m_ui_pass = std::make_shared<UIPass>();
		m_render_passes[ERenderPassType::UI] = m_ui_pass;
		for (auto& render_pass : m_render_passes)
		{
			render_pass.second->init();
		}

		// set vulkan rhi callback functions
		g_runtime_context.eventSystem()->addListener(EventType::RenderCreateSwapchainObjects, 
			std::bind(&RenderSystem::onCreateSwapchainObjects, this, std::placeholders::_1));
		g_runtime_context.eventSystem()->addListener(EventType::RenderDestroySwapchainObjects,
			std::bind(&RenderSystem::onDestroySwapchainObjects, this, std::placeholders::_1));
		g_runtime_context.eventSystem()->addListener(EventType::RenderRecordFrame,
			std::bind(&RenderSystem::onRecordFrame, this, std::placeholders::_1));

		// get dummy texture2d
		const auto& as = g_runtime_context.assetManager();
		m_dummy_texture = as->loadAsset<Texture2D>(DEFAULT_TEXTURE_URL);

		// create lighting uniform buffers
		m_lighting_ubs.resize(MAX_FRAMES_IN_FLIGHT);
		for (VmaBuffer& uniform_buffer : m_lighting_ubs)
		{
			VulkanUtil::createBuffer(sizeof(LightingUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, uniform_buffer);
		}
	}

	void RenderSystem::tick(float delta_time)
	{
		// collect render data from entities of current world
		collectRenderDatas();

		// vulkan rendering
		VulkanRHI::get().render();
	}

	void RenderSystem::destroy()
	{
		for (auto& render_pass : m_render_passes)
		{
			render_pass.second->destroy();
		}
		for (VmaBuffer& uniform_buffer : m_lighting_ubs)
		{
			uniform_buffer.destroy();
		}

		m_dummy_texture.reset();
	}

	std::shared_ptr<RenderPass> RenderSystem::getRenderPass(ERenderPassType render_pass_type)
	{
		return m_render_passes[render_pass_type];
	}

	void RenderSystem::onCreateSwapchainObjects(const std::shared_ptr<class Event>& event)
	{
		const RenderCreateSwapchainObjectsEvent* p_event = static_cast<const RenderCreateSwapchainObjectsEvent*>(event.get());
		m_ui_pass->createResizableObjects(p_event->width, p_event->height);
	}

	void RenderSystem::onDestroySwapchainObjects(const std::shared_ptr<class Event>& event)
	{
		m_ui_pass->destroyResizableObjects();
	}

	void RenderSystem::onRecordFrame(const std::shared_ptr<class Event>& event)
	{
		const RenderRecordFrameEvent* p_event = static_cast<const RenderRecordFrameEvent*>(event.get());

		// ui render pass preparation
		if (m_ui_pass->isEnabled())
		{
			m_ui_pass->prepare();
		}

		// render pass rendering
		for (auto& render_pass : m_render_passes)
		{
			if (render_pass.second->isEnabled())
			{
				render_pass.second->render();
			}
		}
	}

	void RenderSystem::collectRenderDatas()
	{
		// mesh render datas
		std::vector<std::shared_ptr<RenderData>> mesh_render_datas;

		// get current active world
		std::shared_ptr<World> current_world = g_runtime_context.worldManager()->getCurrentWorld();

		// get camera entity
		const auto& camera_entity = current_world->getCameraEntity();
		auto camera_transform_component = camera_entity->getComponent(TransformComponent);
		auto camera_component = camera_entity->getComponent(CameraComponent);

		// update lighting uniform buffers
		LightingUBO lighting_ubo;
		lighting_ubo.camera_pos = camera_transform_component->m_position;
		lighting_ubo.light_dir = glm::vec3(-1.0f, -1.0f, 1.0f);
		lighting_ubo.inv_view_proj = glm::inverse(camera_component->getViewPerspectiveMatrix());
		for (VmaBuffer& uniform_buffer : m_lighting_ubs)
		{
			VulkanUtil::updateBuffer(uniform_buffer, (void*)&lighting_ubo, sizeof(LightingUBO));
		}

		// traverse all entities
		const auto& entities = current_world->getEntities();
		for (const auto& iter : entities)
		{
			const auto& entity = iter.second;

			// get static/skeletal mesh component render data
			auto static_mesh_component = entity->getComponent(StaticMeshComponent);
			auto skeletal_mesh_component = entity->getComponent(SkeletalMeshComponent);

			if (static_mesh_component || skeletal_mesh_component)
			{
				// get transform component
				auto transform_component = entity->getComponent(TransformComponent);

				std::shared_ptr<Mesh> mesh = nullptr;
				if (static_mesh_component)
				{
					mesh = static_mesh_component->getStaticMesh();
				}
				else
				{
					mesh = skeletal_mesh_component->getSkeletalMesh();
				}

				if (mesh)
				{
					// create mesh render data
					EMeshType mesh_type = static_mesh_component ? EMeshType::Static : EMeshType::Skeletal;
					std::shared_ptr<StaticMeshRenderData> static_mesh_render_data = nullptr;
					std::shared_ptr<SkeletalMeshRenderData> skeletal_mesh_render_data = nullptr;

					switch (mesh_type)
					{
					case EMeshType::Static:
					{
						static_mesh_render_data = std::make_shared<StaticMeshRenderData>();
					}
						break;
					case EMeshType::Skeletal:
					{
						skeletal_mesh_render_data = std::make_shared<SkeletalMeshRenderData>();
						static_mesh_render_data = skeletal_mesh_render_data;
					}
						break;
					default:
						break;
					}
					
					static_mesh_render_data->mesh_type = mesh_type;
					static_mesh_render_data->vertex_buffer = mesh->m_vertex_buffer;
					static_mesh_render_data->index_buffer = mesh->m_index_buffer;

					// update uniform buffers
					if (mesh_type == EMeshType::Skeletal)
					{
						skeletal_mesh_render_data->bone_ubs = mesh->m_uniform_buffers;
					}

					// update push constants
					static_mesh_render_data->transform_pco.m = transform_component->getGlobalMatrix();
					static_mesh_render_data->transform_pco.nm = glm::transpose(glm::inverse(glm::mat3(static_mesh_render_data->transform_pco.m)));
					static_mesh_render_data->transform_pco.mvp = camera_component->getViewPerspectiveMatrix() * static_mesh_render_data->transform_pco.m;

					// traverse all sub meshes
					for (size_t i = 0; i < mesh->m_sub_meshes.size(); ++i)
					{
						const auto& sub_mesh = mesh->m_sub_meshes[i];

						static_mesh_render_data->index_counts.push_back(sub_mesh.m_index_count);
						static_mesh_render_data->index_offsets.push_back(sub_mesh.m_index_offset);

						MaterialPCO material_pco;
						material_pco.base_color_factor = sub_mesh.m_material->m_base_color_factor;
						material_pco.emissive_factor = sub_mesh.m_material->m_emissive_factor;
						material_pco.m_metallic_factor = sub_mesh.m_material->m_metallic_factor;
						material_pco.m_roughness_factor = sub_mesh.m_material->m_roughness_factor;
						material_pco.has_base_color_texture = sub_mesh.m_material->m_base_color_texure != nullptr;
						material_pco.has_emissive_texture = sub_mesh.m_material->m_emissive_texure != nullptr;
						material_pco.has_metallic_roughness_texture = sub_mesh.m_material->m_metallic_roughness_texure != nullptr;
						material_pco.has_normal_texture = sub_mesh.m_material->m_normal_texure != nullptr;
						material_pco.has_occlusion_texture = sub_mesh.m_material->m_occlusion_texure != nullptr;
						static_mesh_render_data->material_pcos.push_back(material_pco);

						std::shared_ptr<Texture2D> base_color_texture = sub_mesh.m_material->m_base_color_texure ? sub_mesh.m_material->m_base_color_texure : m_dummy_texture;
						std::shared_ptr<Texture2D> metallic_roughness_texure = sub_mesh.m_material->m_metallic_roughness_texure ? sub_mesh.m_material->m_metallic_roughness_texure : m_dummy_texture;
						std::shared_ptr<Texture2D> normal_texure = sub_mesh.m_material->m_normal_texure ? sub_mesh.m_material->m_normal_texure : m_dummy_texture;
						std::shared_ptr<Texture2D> occlusion_texure = sub_mesh.m_material->m_occlusion_texure ? sub_mesh.m_material->m_occlusion_texure : m_dummy_texture;
						std::shared_ptr<Texture2D> emissive_texture = sub_mesh.m_material->m_emissive_texure ? sub_mesh.m_material->m_emissive_texure : m_dummy_texture;
						static_mesh_render_data->pbr_textures.push_back({
							base_color_texture->m_image_view_sampler,
							metallic_roughness_texure->m_image_view_sampler,
							normal_texure->m_image_view_sampler,
							occlusion_texure->m_image_view_sampler,
							emissive_texture->m_image_view_sampler
						});
					}

					mesh_render_datas.push_back(static_mesh_render_data);
				}
			}
		}

		// set render datas
		std::shared_ptr<LightingRenderData> lighting_render_data = std::make_shared<LightingRenderData>();
		lighting_render_data->lighting_ubs = m_lighting_ubs;

		const auto& sky_light_entity = current_world->getEntity("sky_light");
		auto sky_light_component = sky_light_entity->getComponent(SkyLightComponent);
		lighting_render_data->m_brdf_lut_texture = sky_light_component->m_brdf_lut_texture_sampler;
		lighting_render_data->m_irradiance_texture = sky_light_component->m_irradiance_texture_sampler;
		lighting_render_data->m_prefilter_texture = sky_light_component->m_prefilter_texture_sampler;
		mesh_render_datas.insert(mesh_render_datas.begin(), lighting_render_data);

		//m_render_passes[ERenderPassType::Main]->setRenderDatas(mesh_render_datas);
		m_render_passes[ERenderPassType::Main]->setRenderDatas({ lighting_render_data });
	}

}