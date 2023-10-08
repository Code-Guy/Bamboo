#pragma once

#include <vector>
#include "engine/core/color/color.h"
#include "engine/core/math/transform.h"
#include "engine/core/vulkan/vulkan_util.h"

namespace Bamboo
{
	struct DebugDrawVertex
	{
		glm::vec3 position;
		glm::vec3 color;
	};

	struct DebugDrawLine
	{
		glm::vec3 start;
		glm::vec3 end;
		Color3 color;
	};

	class DebugDrawManager
	{
	public:
		void init();
		void clear();
		void destroy();

		void drawLine(const glm::vec3& start, const glm::vec3& end, const Color3& color = Color3::White);
		void drawLines(const std::vector<DebugDrawLine>& lines);
		void drawBox(const glm::vec3& center, const glm::vec3& extent = k_one_vector, const glm::vec3& rotation = k_zero_vector, const Color3& color = Color3::White);
		void drawSphere(const glm::vec3& center, float radius = 1.0f, uint32_t segment = 12, const Color3& color = Color3::White);
		void drawCylinder(const glm::vec3& start, const glm::vec3& end, float radius = 1.0f, uint32_t segment = 12, const Color3& color = Color3::White);
		void drawCapsule(const glm::vec3& center, float half_height = 2.0f, float radius = 1.0f, const Color3& color = Color3::White);
		void drawFrustum(const glm::mat4& view, const glm::mat4& proj, const Color3& color = Color3::White);

		bool empty() { return m_vertex_count == 0; }
		VkBuffer getVertexBuffer() { return m_vertex_buffer.buffer; }
		uint32_t getVertexCount() { return m_vertex_count; }

	private:
		void update();

		VmaBuffer m_vertex_buffer;
		VmaBuffer m_staging_buffer;
		VkDeviceSize m_max_vertex_buffer_size;

		std::vector<DebugDrawVertex> m_vertices;
		uint32_t m_vertex_count;
		uint32_t m_tick_timer_handle;
	};
}