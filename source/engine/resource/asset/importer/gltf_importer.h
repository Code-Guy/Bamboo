#pragma once

#include <tinygltf/tiny_gltf.h>

#include "import_option.h"
#include "engine/resource/asset/material.h"
#include "engine/resource/asset/static_mesh.h"
#include "engine/resource/asset/skeletal_mesh.h"
#include "engine/resource/asset/skeleton.h"
#include "engine/resource/asset/animation.h"

namespace Bamboo
{
	class GltfImporter
	{
	public:
		static VkFilter getVkFilterFromGltf(int gltf_filter);
		static VkSamplerAddressMode getVkAddressModeFromGltf(int gltf_wrap);
		static glm::mat4 getGltfNodeMatrix(const tinygltf::Node* node);
		static QTranform getGltfNodeTransform(const tinygltf::Node* node);
		static bool validateGltfMeshNode(const tinygltf::Node* node, const tinygltf::Model& gltf_model);
		static bool isGltfSkeletalMesh(const tinygltf::Mesh& gltf_mesh);
		static size_t findGltfJointNodeBoneIndex(const std::vector<std::pair<tinygltf::Node, int>>& joint_nodes, int node_index);
		static uint8_t topologizeGltfBones(std::vector<Bone>& bones, const std::vector<std::pair<tinygltf::Node, int>>& joint_nodes);

		static void importGltfTexture(const tinygltf::Model& gltf_model,
			const tinygltf::Image& gltf_image,
			const tinygltf::Sampler& gltf_sampler,
			uint32_t texture_index,
			std::shared_ptr<Texture2D>& texture);
		static void importGltfPrimitives(const tinygltf::Model& gltf_model,
			const std::vector<std::pair<tinygltf::Primitive, glm::mat4>>& primitives,
			const std::vector<std::shared_ptr<Material>>& materials,
			std::shared_ptr<StaticMesh>& static_mesh,
			std::shared_ptr<SkeletalMesh>& skeletal_mesh);

		static bool importGltf(const std::string& filename, const URL& folder, const GltfImportOption& option);
	};
}