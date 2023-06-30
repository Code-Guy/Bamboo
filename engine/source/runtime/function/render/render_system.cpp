#include "render_system.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/core/vulkan/vulkan_rhi.h"
#include "runtime/function/render/pass/ui_pass.h"
#include "runtime/function/render/pass/base_pass.h"
#include "runtime/function/framework/component/static_mesh_component.h"

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
	}

	void RenderSystem::tick(float delta_time)
	{
		VulkanRHI::get().render();
	}

	void RenderSystem::destroy()
	{
		for (auto& render_pass : m_render_passes)
		{
			render_pass.second->destroy();
		}
	}

	void RenderSystem::setConstructUIFunc(const std::function<void()>& construct_ui_func)
	{
		std::dynamic_pointer_cast<UIPass>(m_render_passes[ERenderPassType::UI])->setConstructFunc(construct_ui_func);
	}

	std::shared_ptr<Bamboo::RenderPass> RenderSystem::getRenderPass(ERenderPassType render_pass_type)
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
		for (auto& render_pass : m_render_passes)
		{
			if (!render_pass.second->isMinimize())
			{
				render_pass.second->record(command_buffer, flight_index);
			}
		}
	}

	void RenderSystem::collectRenderDatas()
	{
		// get current active world
		std::shared_ptr<World> current_world = g_runtime_context.worldManager()->getCurrentWorld();

		// traverse all entities
		const auto& entities = current_world->getEntities();
		for (const auto& iter : entities)
		{
			const auto& entity = iter.second;

			// get static mesh component render data
			StaticMeshComponent* static_mesh_component = entity->getComponent<StaticMeshComponent>();
			if (static_mesh_component)
			{
				const auto& static_mesh = static_mesh_component->getStaticMesh();
				if (static_mesh)
				{
					// traverse all sub meshes
					for (size_t i = 0; i < static_mesh->m_sub_meshes.size(); ++i)
					{
						const auto& sub_mesh = static_mesh->m_sub_meshes[i];

						std::shared_ptr<MeshRenderData> render_data = std::make_shared<MeshRenderData>();
						render_data->vertex_buffer = static_mesh->m_vertex_buffer;
						render_data->index_buffer = static_mesh->m_index_buffer;
						render_data->index_counts.push_back(sub_mesh.m_index_count);
						render_data->index_offsets.push_back(sub_mesh.m_index_offset);

					}
				}
			}
		}
	}

}