#include "render_system.h"
#include "runtime/core/base/macro.h"
#include "runtime/core/event/event_system.h"
#include "runtime/core/math/math_util.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/resource/asset/asset_manager.h"
#include "runtime/function/render/debug_draw_manager.h"
#include "runtime/platform/timer/timer.h"

#include "runtime/core/vulkan/vulkan_rhi.h"
#include "runtime/function/render/pass/directional_light_shadow_pass.h"
#include "runtime/function/render/pass/point_light_shadow_pass.h"
#include "runtime/function/render/pass/spot_light_shadow_pass.h"
#include "runtime/function/render/pass/pick_pass.h"
#include "runtime/function/render/pass/outline_pass.h"
#include "runtime/function/render/pass/main_pass.h"
#include "runtime/function/render/pass/postprocess_pass.h"
#include "runtime/function/render/pass/ui_pass.h"

#include "runtime/function/framework/component/camera_component.h"
#include "runtime/function/framework/component/transform_component.h"
#include "runtime/function/framework/component/static_mesh_component.h"
#include "runtime/function/framework/component/skeletal_mesh_component.h"
#include "runtime/function/framework/component/animator_component.h"
#include "runtime/function/framework/component/sky_light_component.h"
#include "runtime/function/framework/component/directional_light_component.h"
#include "runtime/function/framework/component/point_light_component.h"
#include "runtime/function/framework/component/spot_light_component.h"

namespace Bamboo
{

	void RenderSystem::init()
	{
		m_directional_light_shadow_pass = std::make_shared<DirectionalLightShadowPass>();
		m_point_light_shadow_pass = std::make_shared<PointLightShadowPass>();
		m_spot_light_shadow_pass = std::make_shared<SpotLightShadowPass>();
		m_pick_pass = std::make_shared<PickPass>();
		m_outline_pass = std::make_shared<OutlinePass>();
		m_main_pass = std::make_shared<MainPass>();
		m_postprocess_pass = std::make_shared<class PostprocessPass>();
		m_ui_pass = std::make_shared<UIPass>();

		m_render_passes = {
			m_directional_light_shadow_pass, 
			m_point_light_shadow_pass,
			m_spot_light_shadow_pass,
			m_pick_pass,
			m_outline_pass,
			m_main_pass,
			m_postprocess_pass,
			m_ui_pass
		};
		for (auto& render_pass : m_render_passes)
		{
			render_pass->init();
		}

		// set vulkan rhi callback functions
		g_runtime_context.eventSystem()->addListener(EEventType::RenderCreateSwapchainObjects, 
			std::bind(&RenderSystem::onCreateSwapchainObjects, this, std::placeholders::_1));
		g_runtime_context.eventSystem()->addListener(EEventType::RenderDestroySwapchainObjects,
			std::bind(&RenderSystem::onDestroySwapchainObjects, this, std::placeholders::_1));
		g_runtime_context.eventSystem()->addListener(EEventType::RenderRecordFrame,
			std::bind(&RenderSystem::onRecordFrame, this, std::placeholders::_1));
		g_runtime_context.eventSystem()->addListener(EEventType::PickEntity,
			std::bind(&RenderSystem::onPickEntity, this, std::placeholders::_1));
		g_runtime_context.eventSystem()->addListener(EEventType::SelectEntity,
			std::bind(&RenderSystem::onSelectEntity, this, std::placeholders::_1));

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
		m_lighting_icons = {
			{ ELightType::DirectionalLight, as->loadAsset<Texture2D>("asset/engine/texture/gizmo/tex_directional_light.tex") },
			{ ELightType::SkyLight, as->loadAsset<Texture2D>("asset/engine/texture/gizmo/tex_sky_light.tex") },
			{ ELightType::PointLight, as->loadAsset<Texture2D>("asset/engine/texture/gizmo/tex_point_light.tex") },
			{ ELightType::SpotLight, as->loadAsset<Texture2D>("asset/engine/texture/gizmo/tex_spot_light.tex") }
		};
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
			render_pass->destroy();
		}
		for (VmaBuffer& uniform_buffer : m_lighting_ubs)
		{
			uniform_buffer.destroy();
		}

		for (auto& iter : m_lighting_icons)
		{
			iter.second.reset();
		}

		m_default_texture_2d.reset();
		m_default_texture_cube.reset();
	}

	void RenderSystem::resize(uint32_t width, uint32_t height)
	{
		m_pick_pass->onResize(width, height);
		m_outline_pass->onResize(width, height);
		m_main_pass->onResize(width, height);
		m_postprocess_pass->onResize(width, height);
	}

	VkImageView RenderSystem::getColorImageView()
	{
		return m_postprocess_pass->getColorTexture().view;
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

	void RenderSystem::onPickEntity(const std::shared_ptr<class Event>& event)
	{
		const PickEntityEvent* p_event = static_cast<const PickEntityEvent*>(event.get());
		m_pick_pass->pick(p_event->mouse_x, p_event->mouse_y);
	}

	void RenderSystem::onSelectEntity(const std::shared_ptr<class Event>& event)
	{
		const SelectEntityEvent* p_event = static_cast<const SelectEntityEvent*>(event.get());

		m_selected_entity_ids = { p_event->entity_id };
	}

	void RenderSystem::collectRenderDatas()
	{
		// mesh render datas
		std::vector<std::shared_ptr<RenderData>> mesh_render_datas, selected_mesh_render_datas;
		std::vector<std::shared_ptr<BillboardRenderData>> billboard_render_datas, selected_billboard_render_datas;
		std::vector<uint32_t> mesh_entity_ids, billboard_entity_ids;

		// get current active world
		const auto& current_world = g_runtime_context.worldManager()->getCurrentWorld();

		// get camera entity
		const auto& camera_entity = current_world->getCameraEntity();
		auto camera_transform_component = camera_entity.lock()->getComponent(TransformComponent);
		auto camera_component = camera_entity.lock()->getComponent(CameraComponent);

		// set render datas
		std::shared_ptr<LightingRenderData> lighting_render_data = std::make_shared<LightingRenderData>();
		lighting_render_data->camera_view_proj = camera_component->getViewProjectionMatrix();
		lighting_render_data->brdf_lut_texture = m_default_texture_2d->m_image_view_sampler;
		lighting_render_data->irradiance_texture = m_default_texture_cube->m_image_view_sampler;
		lighting_render_data->prefilter_texture = m_default_texture_cube->m_image_view_sampler;
		lighting_render_data->directional_light_shadow_texture = m_default_texture_2d->m_image_view_sampler;
		lighting_render_data->point_light_shadow_textures.resize(MAX_POINT_LIGHT_NUM);
		lighting_render_data->spot_light_shadow_textures.resize(MAX_SPOT_LIGHT_NUM);
		for (uint32_t i = 0; i < MAX_POINT_LIGHT_NUM; ++i)
		{
			lighting_render_data->point_light_shadow_textures[i] = m_default_texture_cube->m_image_view_sampler;
		}
		for (uint32_t i = 0; i < MAX_SPOT_LIGHT_NUM; ++i)
		{
			lighting_render_data->spot_light_shadow_textures[i] = m_default_texture_2d->m_image_view_sampler;
		}
		std::shared_ptr<SkyboxRenderData> skybox_render_data = nullptr;

		// shadow create infos
		ShadowCascadeCreateInfo shadow_cascade_ci{};
		shadow_cascade_ci.camera_near = camera_component->m_near;
		shadow_cascade_ci.camera_far = camera_component->m_far;
		shadow_cascade_ci.inv_camera_view_proj = glm::inverse(camera_component->getViewProjectionMatrix());

		std::vector<ShadowCubeCreateInfo> shadow_cube_cis;
		std::vector<ShadowFrustumCreateInfo> shadow_frustum_cis;

		// set lighting uniform buffer object
		LightingUBO lighting_ubo;
		lighting_ubo.camera_pos = camera_transform_component->m_position;
		lighting_ubo.camera_dir = camera_transform_component->getForwardVector();
		lighting_ubo.exposure = camera_component->m_exposure;
		lighting_ubo.camera_view = camera_component->getViewMatrix();
		lighting_ubo.inv_camera_view_proj = glm::inverse(camera_component->getViewProjectionMatrix());
		lighting_ubo.has_sky_light = lighting_ubo.has_directional_light = false;
		lighting_ubo.point_light_num = lighting_ubo.spot_light_num = 0;
		lighting_ubo.shader_debug_option = m_shader_debug_option;

		// get debug draw manager
		const auto& ddm = g_runtime_context.debugDrawSystem();
		ddm->clear();

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
					// draw mesh bounding boxes
					BoundingBox bounding_box = mesh->m_bounding_box.transform(transform_component->getGlobalMatrix());
					if ((m_show_debug_option & (1 << 1)) == (1 << 1))
					{
						ddm->drawBox(bounding_box.center(), bounding_box.extent(), k_zero_vector, Color3::Yellow);
					}

					// create mesh render data
					bool is_skeletal_mesh = skeletal_mesh_component != nullptr;
					std::shared_ptr<StaticMeshRenderData> static_mesh_render_data = nullptr;
					std::shared_ptr<SkeletalMeshRenderData> skeletal_mesh_render_data = nullptr;

					if (is_skeletal_mesh)
					{
						skeletal_mesh_render_data = std::make_shared<SkeletalMeshRenderData>();
						static_mesh_render_data = skeletal_mesh_render_data;
					}
					else
					{
						static_mesh_render_data = std::make_shared<StaticMeshRenderData>();
					}

					static_mesh_render_data->type = is_skeletal_mesh ? ERenderDataType::SkeletalMesh : ERenderDataType::StaticMesh;
					static_mesh_render_data->vertex_buffer = mesh->m_vertex_buffer;
					static_mesh_render_data->index_buffer = mesh->m_index_buffer;

					// update uniform buffers
					if (is_skeletal_mesh)
					{
						auto animator_component = entity->getComponent(AnimatorComponent);
						skeletal_mesh_render_data->bone_ubs = animator_component->m_bone_ubs;
					}

					// update push constants
					static_mesh_render_data->transform_pco.m = transform_component->getGlobalMatrix();
					static_mesh_render_data->transform_pco.nm = glm::transpose(glm::inverse(glm::mat3(static_mesh_render_data->transform_pco.m)));
					static_mesh_render_data->transform_pco.mvp = camera_component->getViewProjectionMatrix() * static_mesh_render_data->transform_pco.m;

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

					mesh_render_datas.push_back(static_mesh_render_data);
					if (std::find(m_selected_entity_ids.begin(), m_selected_entity_ids.end(), entity->getID()) != m_selected_entity_ids.end())
					{
						selected_mesh_render_datas.push_back(static_mesh_render_data);
					}
				}

				mesh_entity_ids.push_back(entity->getID());
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
				lighting_ubo.directional_light.cast_shadow = directional_light_component->m_cast_shadow;

				shadow_cascade_ci.light_dir = transform_component->getForwardVector();
				shadow_cascade_ci.light_cascade_frustum_near = directional_light_component->m_cascade_frustum_near;

				addBillboardRenderData(transform_component, camera_component, billboard_render_datas,
					selected_billboard_render_datas, billboard_entity_ids, ELightType::DirectionalLight);
			}

			// get sky light component
			auto sky_light_component = entity->getComponent(SkyLightComponent);
			if (sky_light_component)
			{
				auto transform_component = entity->getComponent(TransformComponent);

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
				skybox_render_data->transform_pco.mvp = camera_component->getProjectionMatrix(EProjectionType::Perspective) * camera_component->getViewMatrixNoTranslation();
				skybox_render_data->env_texture = sky_light_component->m_prefilter_texture_sampler;

				// set lighting uniform buffer object
				lighting_ubo.has_sky_light = true;
				lighting_ubo.sky_light.color = sky_light_component->getColor();
				lighting_ubo.sky_light.prefilter_mip_levels = sky_light_component->m_prefilter_mip_levels;

				addBillboardRenderData(transform_component, camera_component, billboard_render_datas,
					selected_billboard_render_datas, billboard_entity_ids, ELightType::SkyLight);
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
				point_light.cast_shadow = point_light_component->m_cast_shadow;

				ShadowCubeCreateInfo shadow_cube_ci;
				shadow_cube_ci.light_pos = transform_component->m_position;
				shadow_cube_ci.light_far =point_light_component->m_radius;
				shadow_cube_ci.light_near = camera_component->m_near;
				shadow_cube_cis.push_back(shadow_cube_ci);

				addBillboardRenderData(transform_component, camera_component, billboard_render_datas,
					selected_billboard_render_datas, billboard_entity_ids, ELightType::PointLight);
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
				point_light.cast_shadow = spot_light_component->m_cast_shadow;
				point_light.padding0 = std::cos(glm::radians(spot_light_component->m_inner_cone_angle));
				point_light.padding1 = std::cos(glm::radians(spot_light_component->m_outer_cone_angle));

				spot_light.direction = transform_component->getForwardVector();

				ShadowFrustumCreateInfo shadow_frustum_ci;
				shadow_frustum_ci.light_pos = transform_component->m_position;
				shadow_frustum_ci.light_dir = spot_light.direction;
				shadow_frustum_ci.light_angle = spot_light_component->m_outer_cone_angle;
				shadow_frustum_ci.light_far = spot_light_component->m_radius;
				shadow_frustum_ci.light_near = camera_component->m_near;
				shadow_frustum_cis.push_back(shadow_frustum_ci);

				addBillboardRenderData(transform_component, camera_component, billboard_render_datas, 
					selected_billboard_render_datas, billboard_entity_ids, ELightType::SpotLight);
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
			lighting_render_data->directional_light_shadow_texture = m_directional_light_shadow_pass->getShadowImageViewSampler();

			if (lighting_ubo.directional_light.cast_shadow)
			{
				m_directional_light_shadow_pass->setRenderDatas(mesh_render_datas);
			}
		}

		// point light shadow pass: n mesh datas
		if (lighting_ubo.point_light_num > 0)
		{
			m_point_light_shadow_pass->updateCubes(shadow_cube_cis);
			const auto& point_light_shadow_textures = m_point_light_shadow_pass->getShadowImageViewSamplers();
			for (size_t i = 0; i < point_light_shadow_textures.size(); ++i)
			{
				lighting_render_data->point_light_shadow_textures[i] = point_light_shadow_textures[i];
			}

			bool cast_shadow = false;
			for (uint32_t i = 0; i < lighting_ubo.point_light_num; ++i)
			{
				if (lighting_ubo.point_lights[i].cast_shadow)
				{
					cast_shadow = true;
					break;
				}
			}

			if (cast_shadow)
			{
				m_point_light_shadow_pass->setRenderDatas(mesh_render_datas);
			}
		}

		// spot light shadow pass: n mesh datas
		if (lighting_ubo.spot_light_num > 0)
		{
			m_spot_light_shadow_pass->updateFrustums(shadow_frustum_cis);
			const auto& spot_light_shadow_textures = m_spot_light_shadow_pass->getShadowImageViewSamplers();
			for (size_t i = 0; i < spot_light_shadow_textures.size(); ++i)
			{
				lighting_render_data->spot_light_shadow_textures[i] = spot_light_shadow_textures[i];
			}

			bool cast_shadow = false;
			for (uint32_t i = 0; i < lighting_ubo.spot_light_num; ++i)
			{
				lighting_ubo.spot_lights[i].view_proj = m_spot_light_shadow_pass->m_light_view_projs[i];
				if (lighting_ubo.spot_lights[i]._pl.cast_shadow)
				{
					cast_shadow = true;
				}
			}

			if (cast_shadow)
			{
				m_spot_light_shadow_pass->setRenderDatas(mesh_render_datas);
			}
		}

		// update lighting uniform buffers
		VmaBuffer uniform_buffer = m_lighting_ubs[VulkanRHI::get().getFlightIndex()];
		VulkanUtil::updateBuffer(uniform_buffer, (void*)&lighting_ubo, sizeof(LightingUBO));
		lighting_render_data->lighting_ubs = m_lighting_ubs;

		// pick pass
		m_pick_pass->setRenderDatas(mesh_render_datas);
		m_pick_pass->setBillboardRenderDatas(billboard_render_datas);
		mesh_entity_ids.insert(mesh_entity_ids.end(), billboard_entity_ids.begin(), billboard_entity_ids.end());
		m_pick_pass->setEntityIDs(mesh_entity_ids);

		// outline pass
		m_outline_pass->setRenderDatas(selected_mesh_render_datas);
		m_outline_pass->setBillboardRenderDatas(selected_billboard_render_datas);

		// main pass
		m_main_pass->setLightingRenderData(lighting_render_data);
		m_main_pass->setSkyboxRenderData(skybox_render_data);
// 		if (!mesh_render_datas.empty())
// 		{
// 			m_main_pass->setTransparencyRenderDatas({ mesh_render_datas.back() });
// 			mesh_render_datas.pop_back();
// 		}
// 		else
// 		{
// 			m_main_pass->setTransparencyRenderDatas({});
// 		}
		m_main_pass->setBillboardRenderDatas(billboard_render_datas);
		m_main_pass->setRenderDatas(mesh_render_datas);

		// postprocess pass
		std::shared_ptr<PostProcessRenderData> postprocess_render_data = std::make_shared<PostProcessRenderData>();
		postprocess_render_data->p_color_texture = m_main_pass->getColorTexture();
		postprocess_render_data->outline_texture = m_outline_pass->getColorTexture();
		m_postprocess_pass->setRenderDatas({postprocess_render_data});
	}

	void RenderSystem::addBillboardRenderData(
		std::shared_ptr<class TransformComponent> transform_component,
		std::shared_ptr<class CameraComponent> camera_component,
		std::vector<std::shared_ptr<BillboardRenderData>>& billboard_render_datas,
		std::vector<std::shared_ptr<BillboardRenderData>>& selected_billboard_render_datas,
		std::vector<uint32_t>& billboard_entity_ids,
		ELightType light_type)
	{
		std::shared_ptr<BillboardRenderData> billboard_render_data = std::make_shared<BillboardRenderData>();
		const glm::vec3& billboard_pos = transform_component->m_position;
		const glm::vec3& camera_pos = camera_component->getPosition();

		const float k_min_size = 0.005f;
		const float k_max_size = 0.05f;
		const float k_min_dist = 5.0f;
		const float k_max_dist = 100.0f;

		float dist = glm::distance(billboard_pos, camera_pos);
		float size = MathUtil::mapRangeValueClamped(dist, k_min_dist, k_max_dist, k_max_size, k_min_size);
		billboard_render_data->position = camera_component->getViewProjectionMatrix() * glm::vec4(billboard_pos, 1.0f);
		billboard_render_data->position /= billboard_render_data->position.w;
		billboard_render_data->size = glm::vec2(size, size * camera_component->m_aspect_ratio);
		billboard_render_data->texture = m_lighting_icons[light_type]->m_image_view_sampler;

		uint32_t entity_id = transform_component->getParent().lock()->getID();
		billboard_render_datas.push_back(billboard_render_data);
		if (std::find(m_selected_entity_ids.begin(), m_selected_entity_ids.end(), entity_id) != m_selected_entity_ids.end())
		{
			selected_billboard_render_datas.push_back(billboard_render_data);
		}
		billboard_entity_ids.push_back(entity_id);
	}

}