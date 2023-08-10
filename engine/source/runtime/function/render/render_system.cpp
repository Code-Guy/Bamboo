#include "render_system.h"
#include "runtime/core/base/macro.h"
#include "runtime/core/event/event_system.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/resource/asset/asset_manager.h"

#include "runtime/core/vulkan/vulkan_rhi.h"
#include "runtime/function/render/pass/brdf_lut_pass.h"
#include "runtime/function/render/pass/filter_cube_pass.h"
#include "runtime/function/render/pass/base_pass.h"
#include "runtime/function/render/pass/ui_pass.h"

#include "runtime/function/framework/component/camera_component.h"
#include "runtime/function/framework/component/transform_component.h"
#include "runtime/function/framework/component/static_mesh_component.h"
#include "runtime/function/framework/component/skeletal_mesh_component.h"

namespace Bamboo
{

	void RenderSystem::init()
	{		
		// init persistent render passes
		m_render_passes[ERenderPassType::Base] = std::make_shared<BasePass>();
		m_ui_pass = std::make_shared<UIPass>();
		m_render_passes[ERenderPassType::UI] = m_ui_pass;
		for (auto& render_pass : m_render_passes)
		{
			render_pass.second->init();
		}

		// init instant render passes
		const auto& as = g_runtime_context.assetManager();
		if (g_runtime_context.fileSystem()->exists(BRDF_TEX_URL))
		{
			
		}
		else
		{
			std::shared_ptr<BRDFLUTPass> brdf_pass = std::make_shared<BRDFLUTPass>();
			brdf_pass->init();
			brdf_pass->createResizableObjects(2048, 2048);
			brdf_pass->render();
			brdf_pass->destroy();
		}

		std::shared_ptr<FilterCubePass> filter_cube_pass = std::make_shared<FilterCubePass>();
		filter_cube_pass->init();
		filter_cube_pass->createResizableObjects(0, 0);
		filter_cube_pass->render();
		filter_cube_pass->destroy();

		// set vulkan rhi callback functions
		g_runtime_context.eventSystem()->addListener(EventType::RenderCreateSwapchainObjects, 
			std::bind(&RenderSystem::onCreateSwapchainObjects, this, std::placeholders::_1));
		g_runtime_context.eventSystem()->addListener(EventType::RenderDestroySwapchainObjects,
			std::bind(&RenderSystem::onDestroySwapchainObjects, this, std::placeholders::_1));
		g_runtime_context.eventSystem()->addListener(EventType::RenderRecordFrame,
			std::bind(&RenderSystem::onRecordFrame, this, std::placeholders::_1));

		// get dummy texture2d
		m_dummy_texture = as->loadAsset<Texture2D>("asset/engine/texture/material/tex_dummy.tex");

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
					std::shared_ptr<MeshRenderData> mesh_render_data = nullptr;
					std::shared_ptr<SkeletalRenderData> skeletal_mesh_render_data = nullptr;

					switch (mesh_type)
					{
					case EMeshType::Static:
					{
						mesh_render_data = std::make_shared<MeshRenderData>();
					}
						break;
					case EMeshType::Skeletal:
					{
						skeletal_mesh_render_data = std::make_shared<SkeletalRenderData>();
						mesh_render_data = skeletal_mesh_render_data;
					}
						break;
					default:
						break;
					}
					
					mesh_render_data->mesh_type = mesh_type;
					mesh_render_data->vertex_buffer = mesh->m_vertex_buffer;
					mesh_render_data->index_buffer = mesh->m_index_buffer;

					// update uniform buffers
					mesh_render_data->lighting_ubs = m_lighting_ubs;
					if (mesh_type == EMeshType::Skeletal)
					{
						skeletal_mesh_render_data->bone_ubs = mesh->m_uniform_buffers;
					}

					// update push constants
					mesh_render_data->transform_pco.m = transform_component->getGlobalMatrix();
					mesh_render_data->transform_pco.nm = glm::transpose(glm::inverse(glm::mat3(mesh_render_data->transform_pco.m)));
					mesh_render_data->transform_pco.mvp = camera_component->getViewPerspectiveMatrix() * mesh_render_data->transform_pco.m;

					// traverse all sub meshes
					for (size_t i = 0; i < mesh->m_sub_meshes.size(); ++i)
					{
						const auto& sub_mesh = mesh->m_sub_meshes[i];

						mesh_render_data->index_counts.push_back(sub_mesh.m_index_count);
						mesh_render_data->index_offsets.push_back(sub_mesh.m_index_offset);

						MaterialUBO material_pco;
						material_pco.base_color_factor = sub_mesh.m_material->m_base_color_factor;
						material_pco.has_base_color_texture = sub_mesh.m_material->m_base_color_texure != nullptr;
						material_pco.emissive_factor = sub_mesh.m_material->m_emissive_factor;
						material_pco.has_emissive_texture = sub_mesh.m_material->m_emissive_texure != nullptr;
						material_pco.m_metallic_factor = sub_mesh.m_material->m_metallic_factor;
						material_pco.m_roughness_factor = sub_mesh.m_material->m_roughness_factor;
						mesh_render_data->material_pcos.push_back(material_pco);

						std::shared_ptr<Texture2D> base_color_texture = sub_mesh.m_material->m_base_color_texure ? sub_mesh.m_material->m_base_color_texure : m_dummy_texture;
						std::shared_ptr<Texture2D> emissive_texture = sub_mesh.m_material->m_emissive_texure ? sub_mesh.m_material->m_emissive_texure : m_dummy_texture;
						mesh_render_data->textures.push_back(base_color_texture->m_image_view_sampler);
					}

					mesh_render_datas.push_back(mesh_render_data);
				}
			}
		}

		// set render datas
		m_render_passes[ERenderPassType::Base]->setRenderDatas(mesh_render_datas);
	}

}