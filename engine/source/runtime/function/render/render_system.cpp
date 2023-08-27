#include "render_system.h"
#include "runtime/core/base/macro.h"
#include "runtime/core/event/event_system.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/resource/asset/asset_manager.h"
#include "runtime/platform/timer/timer.h"

#include "runtime/core/vulkan/vulkan_rhi.h"
#include "runtime/function/render/pass/directional_light_shadow_pass.h"
#include "runtime/function/render/pass/main_pass.h"
#include "runtime/function/render/pass/ui_pass.h"

#include "runtime/function/framework/component/camera_component.h"
#include "runtime/function/framework/component/transform_component.h"
#include "runtime/function/framework/component/static_mesh_component.h"
#include "runtime/function/framework/component/skeletal_mesh_component.h"
#include "runtime/function/framework/component/sky_light_component.h"
#include "runtime/function/framework/component/directional_light_component.h"
#include "runtime/function/framework/component/point_light_component.h"
#include "runtime/function/framework/component/spot_light_component.h"

namespace Bamboo
{

	void RenderSystem::init()
	{
		m_directional_light_shadow_pass = std::make_shared<DirectionalLightShadowPass>();
		m_main_pass = std::make_shared<MainPass>();
		m_ui_pass = std::make_shared<UIPass>();

		m_render_passes = {
			m_directional_light_shadow_pass, m_main_pass, m_ui_pass
		};
		for (auto& render_pass : m_render_passes)
		{
			render_pass->init();
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
		m_default_texture_2d = as->loadAsset<Texture2D>(DEFAULT_TEXTURE_2D_URL);
		m_default_texture_cube = as->loadAsset<TextureCube>(DEFAULT_TEXTURE_CUBE_URL);

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
// 		StopWatch stop_watch;
// 		stop_watch.start();

		collectRenderDatas();

		//LOG_INFO("collect render datas elapsed time: {}ms", stop_watch.stopHP());

		// vulkan rendering
		VulkanRHI::get().render();
	}

	void RenderSystem::destroy()
	{
		for (auto& render_pass : m_render_passes)
		{
			render_pass->destroy();
		}
		for (VmaBuffer& uniform_buffer : m_lighting_ubs)
		{
			uniform_buffer.destroy();
		}

		m_default_texture_2d.reset();
		m_default_texture_cube.reset();
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
			if (render_pass->isEnabled())
			{
				render_pass->render();
			}
		}
	}

	void RenderSystem::collectRenderDatas()
	{
		// mesh render datas
		std::vector<std::shared_ptr<RenderData>> directional_light_shadow_pass_render_datas;
		std::vector<std::shared_ptr<RenderData>> main_pass_render_datas;

		// get current active world
		std::shared_ptr<World> current_world = g_runtime_context.worldManager()->getCurrentWorld();

		// get camera entity
		const auto& camera_entity = current_world->getCameraEntity();
		auto camera_transform_component = camera_entity->getComponent(TransformComponent);
		auto camera_component = camera_entity->getComponent(CameraComponent);

		// set render datas
		std::shared_ptr<LightingRenderData> lighting_render_data = std::make_shared<LightingRenderData>();
		lighting_render_data->brdf_lut_texture = m_default_texture_2d->m_image_view_sampler;
		lighting_render_data->irradiance_texture = m_default_texture_cube->m_image_view_sampler;
		lighting_render_data->prefilter_texture = m_default_texture_cube->m_image_view_sampler;

		std::shared_ptr<SkyboxRenderData> skybox_render_data = nullptr;
		ShadowCascadeCreateInfo shadow_cascade_ci{};
		shadow_cascade_ci.camera_near = camera_component->m_near;
		shadow_cascade_ci.camera_far = camera_component->m_far;
		shadow_cascade_ci.inv_camera_view_proj = glm::inverse(camera_component->getViewPerspectiveMatrix());

		// set lighting uniform buffer object
		LightingUBO lighting_ubo;
		lighting_ubo.camera_pos = camera_transform_component->m_position;
		lighting_ubo.camera_view = camera_component->getViewMatrix();
		lighting_ubo.inv_camera_view_proj = glm::inverse(camera_component->getViewPerspectiveMatrix());
		lighting_ubo.has_sky_light = lighting_ubo.has_directional_light = false;
		lighting_ubo.point_light_num = lighting_ubo.spot_light_num = 0;

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
						material_pco.has_metallic_roughness_occlusion_texture = sub_mesh.m_material->m_metallic_roughness_occlusion_texure != nullptr;
						material_pco.contains_occlusion_channel = sub_mesh.m_material->m_contains_occlusion_channel;
						material_pco.has_normal_texture = sub_mesh.m_material->m_normal_texure != nullptr;
						static_mesh_render_data->material_pcos.push_back(material_pco);

						std::shared_ptr<Texture2D> base_color_texture = sub_mesh.m_material->m_base_color_texure ? sub_mesh.m_material->m_base_color_texure : m_default_texture_2d;
						std::shared_ptr<Texture2D> metallic_roughness_occlusion_texure = sub_mesh.m_material->m_metallic_roughness_occlusion_texure ? sub_mesh.m_material->m_metallic_roughness_occlusion_texure : m_default_texture_2d;
						std::shared_ptr<Texture2D> normal_texure = sub_mesh.m_material->m_normal_texure ? sub_mesh.m_material->m_normal_texure : m_default_texture_2d;
						std::shared_ptr<Texture2D> emissive_texture = sub_mesh.m_material->m_emissive_texure ? sub_mesh.m_material->m_emissive_texure : m_default_texture_2d;
						static_mesh_render_data->pbr_textures.push_back({
							base_color_texture->m_image_view_sampler,
							metallic_roughness_occlusion_texure->m_image_view_sampler,
							normal_texure->m_image_view_sampler,
							emissive_texture->m_image_view_sampler
						});
					}

					directional_light_shadow_pass_render_datas.push_back(static_mesh_render_data);
					main_pass_render_datas.push_back(static_mesh_render_data);
				}
			}

			// get sky light component
			auto sky_light_component = entity->getComponent(SkyLightComponent);
			if (sky_light_component)
			{
				// set lighting render data
				lighting_render_data->brdf_lut_texture = sky_light_component->m_brdf_lut_texture_sampler;
				lighting_render_data->irradiance_texture = sky_light_component->m_irradiance_texture_sampler;
				lighting_render_data->prefilter_texture = sky_light_component->m_prefilter_texture_sampler;

				// set skybox render data
				skybox_render_data = std::make_shared<SkyboxRenderData>();
				std::shared_ptr<StaticMesh> skybox_cube_mesh = sky_light_component->m_cube_mesh;
				skybox_render_data->vertex_buffer = skybox_cube_mesh->m_vertex_buffer;
				skybox_render_data->index_buffer = skybox_cube_mesh->m_index_buffer;
				skybox_render_data->index_count = skybox_cube_mesh->m_sub_meshes.front().m_index_count;
				skybox_render_data->transform_pco.mvp = camera_component->getPerspectiveMatrix() * glm::mat4(glm::mat3(camera_component->getViewMatrix()));
				skybox_render_data->env_texture = sky_light_component->m_prefilter_texture_sampler;

				// set lighting uniform buffer object
				lighting_ubo.has_sky_light = true;
				lighting_ubo.sky_light.color = sky_light_component->getColor();
				lighting_ubo.sky_light.prefilter_mip_levels = sky_light_component->m_prefilter_mip_levels;
			}

			// get directional light component
			auto directional_light_component = entity->getComponent(DirectionalLightComponent);
			if (directional_light_component)
			{
				auto transform_component = entity->getComponent(TransformComponent);

				// set lighting uniform buffer object
				lighting_ubo.has_directional_light = true;
				lighting_ubo.directional_light.direction = transform_component->getForwardVector();
				lighting_ubo.directional_light.color = directional_light_component->getColor();

				shadow_cascade_ci.light_dir = transform_component->getForwardVector();
				shadow_cascade_ci.light_cascade_frustum_near = directional_light_component->m_cascade_frustum_near;
			}

			// get point light component
			auto point_light_component = entity->getComponent(PointLightComponent);
			if (point_light_component)
			{
				auto transform_component = entity->getComponent(TransformComponent);

				// set lighting uniform buffer object
				PointLight& point_light = lighting_ubo.point_lights[lighting_ubo.point_light_num++];
				point_light.position = transform_component->m_position;
				point_light.color = point_light_component->getColor();
				point_light.radius = point_light_component->m_radius;
				point_light.linear_attenuation = point_light_component->m_linear_attenuation;
				point_light.quadratic_attenuation = point_light_component->m_quadratic_attenuation;
			}

			// get point light component
			auto spot_light_component = entity->getComponent(SpotLightComponent);
			if (spot_light_component)
			{
				auto transform_component = entity->getComponent(TransformComponent);

				// set lighting uniform buffer object
				SpotLight& spot_light = lighting_ubo.spot_lights[lighting_ubo.spot_light_num++];
				PointLight& point_light = spot_light._pl;
				point_light.position = transform_component->m_position;
				point_light.color = spot_light_component->getColor();
				point_light.radius = spot_light_component->m_radius;
				point_light.linear_attenuation = spot_light_component->m_linear_attenuation;
				point_light.quadratic_attenuation = spot_light_component->m_quadratic_attenuation;
				point_light.padding0 = std::cos(glm::radians(spot_light_component->m_inner_cone_angle));
				point_light.padding1 = std::cos(glm::radians(spot_light_component->m_outer_cone_angle));

				spot_light.direction = transform_component->getForwardVector();
			}
		}

		// directional light shadow pass: n mesh datas
		if (lighting_ubo.has_directional_light)
		{
			m_directional_light_shadow_pass->updateCascades(shadow_cascade_ci);

			for (uint32_t i = 0; i < SHADOW_CASCADE_NUM; ++i)
			{
				lighting_ubo.directional_light.cascade_splits[i] = m_directional_light_shadow_pass->m_cascade_splits[i];
				lighting_ubo.directional_light.cascade_view_projs[i] = m_directional_light_shadow_pass->m_shadow_cascade_ubo.cascade_view_projs[i];
			}
			lighting_render_data->directional_light_shadow_texture = m_directional_light_shadow_pass->getDepthImageViewSampler();

			m_directional_light_shadow_pass->setRenderDatas(directional_light_shadow_pass_render_datas);
		}
		
		// update lighting uniform buffers
		for (VmaBuffer& uniform_buffer : m_lighting_ubs)
		{
			VulkanUtil::updateBuffer(uniform_buffer, (void*)&lighting_ubo, sizeof(LightingUBO));
		}
		lighting_render_data->lighting_ubs = m_lighting_ubs;

		// main pass: 1 light data + n mesh datas(opaque or transparent) + 1 skybox render data(optional)
		main_pass_render_datas.insert(main_pass_render_datas.begin(), lighting_render_data);
		if (skybox_render_data)
		{
			main_pass_render_datas.push_back(skybox_render_data);
		}
		m_main_pass->setRenderDatas(main_pass_render_datas);
	}

}