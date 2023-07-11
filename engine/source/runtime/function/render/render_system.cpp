#include "render_system.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/resource/asset/asset_manager.h"

#include "runtime/core/vulkan/vulkan_rhi.h"
#include "runtime/function/render/pass/ui_pass.h"
#include "runtime/function/render/pass/base_pass.h"
#include "runtime/function/framework/component/camera_component.h"
#include "runtime/function/framework/component/transform_component.h"
#include "runtime/function/framework/component/static_mesh_component.h"
#include "runtime/function/framework/component/skeletal_mesh_component.h"

namespace Bamboo
{

	void RenderSystem::init()
	{
		m_render_passes[ERenderPassType::Base] = std::make_shared<BasePass>();
		m_render_passes[ERenderPassType::UI] = std::make_shared<UIPass>();
		for (auto& render_pass : m_render_passes)
		{
			render_pass.second->init();
		}

		// set vulkan rhi callback functions
		VulkanRHI::get().setCallbacks({
			std::bind(&RenderSystem::onCreateSwapchainObjects, this, std::placeholders::_1, std::placeholders::_2),
			std::bind(&RenderSystem::onDestroySwapchainObjects, this),
			std::bind(&RenderSystem::onRecordFrame, this, std::placeholders::_1, std::placeholders::_2),
			});

		// get dummy texture2d
		m_dummy_texture = g_runtime_context.assetManager()->loadAsset<Texture2D>("asset/engine/texture/material/tex_dummy.tex");
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
		m_dummy_texture.reset();
	}

	void RenderSystem::setConstructUIFunc(const std::function<void()>& construct_ui_func)
	{
		std::dynamic_pointer_cast<UIPass>(m_render_passes[ERenderPassType::UI])->setConstructFunc(construct_ui_func);
	}

	std::shared_ptr<RenderPass> RenderSystem::getRenderPass(ERenderPassType render_pass_type)
	{
		return m_render_passes[render_pass_type];
	}

	void RenderSystem::onCreateSwapchainObjects(uint32_t width, uint32_t height)
	{
		if (m_render_passes.find(ERenderPassType::UI) != m_render_passes.end())
		{
			m_render_passes[ERenderPassType::UI]->createResizableObjects(width, height);
		}
	}

	void RenderSystem::onDestroySwapchainObjects()
	{
		if (m_render_passes.find(ERenderPassType::UI) != m_render_passes.end())
		{
			m_render_passes[ERenderPassType::UI]->destroyResizableObjects();
		}
	}

	void RenderSystem::onRecordFrame(VkCommandBuffer command_buffer, uint32_t flight_index)
	{
		// render pass preparation
		for (auto& render_pass : m_render_passes)
		{
			if (!render_pass.second->isMinimize())
			{
				render_pass.second->prepare();
			}
		}

		// render pass rendering
		for (auto& render_pass : m_render_passes)
		{
			if (!render_pass.second->isMinimize())
			{
				render_pass.second->render(command_buffer, flight_index);
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
		auto camera_component = camera_entity->getComponent<CameraComponent>();

		// traverse all entities
		const auto& entities = current_world->getEntities();
		for (const auto& iter : entities)
		{
			const auto& entity = iter.second;

			// get static/skeletal mesh component render data
			auto static_mesh_component = entity->getComponent<StaticMeshComponent>();
			auto skeletal_mesh_component = entity->getComponent<SkeletalMeshComponent>();

			if (static_mesh_component || skeletal_mesh_component)
			{
				// get transform component
				auto transform_component = entity->getComponent<TransformComponent>();

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
					std::shared_ptr<MeshRenderData> render_data = std::make_shared<MeshRenderData>();
					render_data->mesh_type = static_mesh_component ? EMeshType::Static : EMeshType::Skeletal;

					render_data->vertex_buffer = mesh->m_vertex_buffer;
					render_data->index_buffer = mesh->m_index_buffer;
					render_data->uniform_buffers = mesh->m_uniform_buffers;

					// set push constants
					render_data->vert_pco.m = transform_component->world_matrix;
					render_data->vert_pco.mvp = camera_component->getViewPerspectiveMatrix() * transform_component->world_matrix;

					FragPCO frag_pco;
					frag_pco.camera_pos = camera_component->m_position;
					frag_pco.light_dir = glm::vec3(-1.0f, -1.0f, 1.0f);
					
					// traverse all sub meshes
					for (size_t i = 0; i < mesh->m_sub_meshes.size(); ++i)
					{
						const auto& sub_mesh = mesh->m_sub_meshes[i];

						render_data->index_counts.push_back(sub_mesh.m_index_count);
						render_data->index_offsets.push_back(sub_mesh.m_index_offset);

						frag_pco.base_color_factor = sub_mesh.m_material->m_base_color_factor;
						frag_pco.has_base_color_texture = sub_mesh.m_material->m_base_color_texure != nullptr;
						frag_pco.emissive_factor = sub_mesh.m_material->m_emissive_factor;
						frag_pco.has_emissive_texture = sub_mesh.m_material->m_emissive_texure != nullptr;
						frag_pco.m_metallic_factor = sub_mesh.m_material->m_metallic_factor;
						frag_pco.m_roughness_factor = sub_mesh.m_material->m_roughness_factor;
						render_data->frag_pcos.push_back(frag_pco);

						std::shared_ptr<Texture2D> base_color_texture = sub_mesh.m_material->m_base_color_texure ? sub_mesh.m_material->m_base_color_texure : m_dummy_texture;
						std::shared_ptr<Texture2D> emissive_texture = sub_mesh.m_material->m_emissive_texure ? sub_mesh.m_material->m_emissive_texure : m_dummy_texture;
						render_data->textures.push_back(base_color_texture->m_image_view_sampler);
					}

					mesh_render_datas.push_back(render_data);
				}
			}
		}

		// set render datas
		m_render_passes[ERenderPassType::Base]->setRenderDatas(mesh_render_datas);
	}

}