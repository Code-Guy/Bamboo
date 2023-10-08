#include "debug_draw_manager.h"
#include "engine/core/base/macro.h"
#include "engine/platform/timer/timer.h"

#define MAX_VERTEX_COUNT 1024
#define UPDATE_BUFFER_FPS 30

namespace Bamboo
{
	
	void DebugDrawManager::init()
	{
		m_vertex_count = 0;
		m_max_vertex_buffer_size = MAX_VERTEX_COUNT * sizeof(DebugDrawVertex);
		VulkanUtil::createBuffer(m_max_vertex_buffer_size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
			m_vertex_buffer);

		VulkanUtil::createBuffer(m_max_vertex_buffer_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
			m_staging_buffer);

		m_tick_timer_handle = g_engine.timerManager()->addTimer(1.0f / UPDATE_BUFFER_FPS, [this]() { update(); }, true);
	}

	void DebugDrawManager::clear()
	{
		// clear vertices
		m_vertices.clear();
	}

	void DebugDrawManager::destroy()
	{
		m_vertex_buffer.destroy();
		m_staging_buffer.destroy();
		g_engine.timerManager()->removeTimer(m_tick_timer_handle);
	}

	void DebugDrawManager::drawLine(const glm::vec3& start, const glm::vec3& end, const Color3& color /*= Color3::White*/)
	{
		m_vertices.push_back({ start, color.toVec3() });
		m_vertices.push_back({ end, color.toVec3() });
	}

	void DebugDrawManager::drawLines(const std::vector<DebugDrawLine>& lines)
	{
		for (const auto& line : lines)
		{
			drawLine(line.start, line.end, line.color);
		}
	}

	void DebugDrawManager::drawBox(const glm::vec3& center, const glm::vec3& extent /*= glm::vec3(1.0f)*/, const glm::vec3& rotation /*= glm::vec3(0.0f)*/, const Color3& color /*= Color3::White*/)
	{
		if (rotation == k_zero_vector)
		{
			drawLine(center + glm::vec3(extent.x, extent.y, extent.z), center + glm::vec3(extent.x, -extent.y, extent.z), color);
			drawLine(center + glm::vec3(extent.x, -extent.y, extent.z), center + glm::vec3(-extent.x, -extent.y, extent.z), color);
			drawLine(center + glm::vec3(-extent.x, -extent.y, extent.z), center + glm::vec3(-extent.x, extent.y, extent.z), color);
			drawLine(center + glm::vec3(-extent.x, extent.y, extent.z), center + glm::vec3(extent.x, extent.y, extent.z), color);

			drawLine(center + glm::vec3(extent.x, extent.y, -extent.z), center + glm::vec3(extent.x, -extent.y, -extent.z), color);
			drawLine(center + glm::vec3(extent.x, -extent.y, -extent.z), center + glm::vec3(-extent.x, -extent.y, -extent.z), color);
			drawLine(center + glm::vec3(-extent.x, -extent.y, -extent.z), center + glm::vec3(-extent.x, extent.y, -extent.z), color);
			drawLine(center + glm::vec3(-extent.x, extent.y, -extent.z), center + glm::vec3(extent.x, extent.y, -extent.z), color);

			drawLine(center + glm::vec3(extent.x, extent.y, extent.z), center + glm::vec3(extent.x, extent.y, -extent.z), color);
			drawLine(center + glm::vec3(extent.x, -extent.y, extent.z), center + glm::vec3(extent.x, -extent.y, -extent.z), color);
			drawLine(center + glm::vec3(-extent.x, -extent.y, extent.z), center + glm::vec3(-extent.x, -extent.y, -extent.z), color);
			drawLine(center + glm::vec3(-extent.x, extent.y, extent.z), center + glm::vec3(-extent.x, extent.y, -extent.z), color);
		}
		else
		{
			Transform transform;
			transform.m_rotation = rotation;
			glm::vec3 start = transform.transformPosition(glm::vec3(extent.x, extent.y, extent.z));
			glm::vec3 end = transform.transformPosition(glm::vec3(extent.x, -extent.y, extent.z));
			drawLine(center + start, center + end, color);

			start = transform.transformPosition(glm::vec3(extent.x, -extent.y, extent.z));
			end = transform.transformPosition(glm::vec3(-extent.x, -extent.y, extent.z));
			drawLine(center + start, center + end, color);

			start = transform.transformPosition(glm::vec3(-extent.x, -extent.y, extent.z));
			end = transform.transformPosition(glm::vec3(-extent.x, extent.y, extent.z));
			drawLine(center + start, center + end, color);

			start = transform.transformPosition(glm::vec3(-extent.x, extent.y, extent.z));
			end = transform.transformPosition(glm::vec3(extent.x, extent.y, extent.z));
			drawLine(center + start, center + end, color);

			start = transform.transformPosition(glm::vec3(extent.x, extent.y, -extent.z));
			end = transform.transformPosition(glm::vec3(extent.x, -extent.y, -extent.z));
			drawLine(center + start, center + end, color);

			start = transform.transformPosition(glm::vec3(extent.x, -extent.y, -extent.z));
			end = transform.transformPosition(glm::vec3(-extent.x, -extent.y, -extent.z));
			drawLine(center + start, center + end, color);

			start = transform.transformPosition(glm::vec3(-extent.x, -extent.y, -extent.z));
			end = transform.transformPosition(glm::vec3(-extent.x, extent.y, -extent.z));
			drawLine(center + start, center + end, color);

			start = transform.transformPosition(glm::vec3(-extent.x, extent.y, -extent.z));
			end = transform.transformPosition(glm::vec3(extent.x, extent.y, -extent.z));
			drawLine(center + start, center + end, color);

			start = transform.transformPosition(glm::vec3(extent.x, extent.y, extent.z));
			end = transform.transformPosition(glm::vec3(extent.x, extent.y, -extent.z));
			drawLine(center + start, center + end, color);

			start = transform.transformPosition(glm::vec3(extent.x, -extent.y, extent.z));
			end = transform.transformPosition(glm::vec3(extent.x, -extent.y, -extent.z));
			drawLine(center + start, center + end, color);

			start = transform.transformPosition(glm::vec3(-extent.x, -extent.y, extent.z));
			end = transform.transformPosition(glm::vec3(-extent.x, -extent.y, -extent.z));
			drawLine(center + start, center + end, color);

			start = transform.transformPosition(glm::vec3(-extent.x, extent.y, extent.z));
			end = transform.transformPosition(glm::vec3(-extent.x, extent.y, -extent.z));
			drawLine(center + start, center + end, color);
		}
	}

	void DebugDrawManager::drawSphere(const glm::vec3& center, float radius /*= 1.0f*/, uint32_t segment /*= 12*/, const Color3& color /*= Color3::White*/)
	{

	}

	void DebugDrawManager::drawCylinder(const glm::vec3& start, const glm::vec3& end, float radius /*= 1.0f*/, uint32_t segment /*= 12*/, const Color3& color /*= Color3::White*/)
	{

	}

	void DebugDrawManager::drawCapsule(const glm::vec3& center, float half_height /*= 2.0f*/, float radius /*= 1.0f*/, const Color3& color /*= Color3::White*/)
	{

	}

	void DebugDrawManager::drawFrustum(const glm::mat4& view, const glm::mat4& proj, const Color3& color /*= Color3::White*/)
	{

	}

	void DebugDrawManager::update()
	{
		// update vertex buffer
		m_vertex_count = static_cast<uint32_t>(m_vertices.size());
		if (m_vertex_count > 0)
		{
			VkDeviceSize vertex_buffer_size = m_vertex_count * sizeof(DebugDrawVertex);
			ASSERT(vertex_buffer_size < m_max_vertex_buffer_size, "debug draw vertex overflow");
			VulkanUtil::updateBuffer(m_staging_buffer, m_vertices.data(), vertex_buffer_size);
			VulkanUtil::copyBuffer(m_staging_buffer.buffer, m_vertex_buffer.buffer, vertex_buffer_size);
		}
	}

}