#include "asset_manager.h"
#include "runtime/resource/asset/texture_2d.h"
#include "runtime/resource/asset/material.h"
#include "runtime/resource/asset/static_mesh.h"
#include "runtime/resource/asset/skeletal_mesh.h"
#include "runtime/resource/asset/skeleton.h"
#include "runtime/resource/asset/animation.h"
#include "runtime/function/framework/world/world.h"

#include <fstream>
#include <queue>
#include <algorithm>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>

#define REFERENCE_ASSET(object, prop_name, ref_asset) \
	object-> ##prop_name = ref_asset; \
	object->m_ref_urls[#prop_name] = ref_asset->getURL()

#define IMAGE_COMPONENT 4
#define IMAGE_BIT_DEPTH 8

namespace Bamboo
{
	void AssetManager::init()
	{
		m_asset_type_exts = {
			{ EAssetType::Texture2D, "tex" },
			{ EAssetType::TextureCube, "texc" }, 
			{ EAssetType::Material, "mat" }, 
			{ EAssetType::Skeleton, "skl" },
			{ EAssetType::StaticMesh, "sm"}, 
			{ EAssetType::SkeletalMesh, "skm" }, 
			{ EAssetType::Animation, "anim" },
			{ EAssetType::World, "world" },
			{ EAssetType::Font, "ttf" }
		};

		m_asset_archive_types = {
			{ EAssetType::Texture2D, EArchiveType::Binary },
			{ EAssetType::TextureCube, EArchiveType::Binary },
			{ EAssetType::Material, EArchiveType::Json },
			{ EAssetType::Skeleton, EArchiveType::Binary },
			{ EAssetType::StaticMesh, EArchiveType::Binary },
			{ EAssetType::SkeletalMesh, EArchiveType::Binary },
			{ EAssetType::Animation, EArchiveType::Binary },
			{ EAssetType::World, EArchiveType::Json }
		};

		for (const auto& iter : m_asset_type_exts)
		{
			m_ext_asset_types[iter.second] = iter.first;
		}
	}

	void AssetManager::destroy()
	{
		for (auto& iter : m_assets)
		{
			iter.second.reset();
		}
		m_assets.clear();
	}

	bool AssetManager::importAsset(const std::string& filename, const URL& folder)
	{
		std::string extension = g_runtime_context.fileSystem()->extension(filename);
		if (extension == "glb" || extension == "gltf")
		{
			return importGltf(filename, folder);
		}
		else
		{
			LOG_WARNING("unknown asset format");
			return false;
		}
	}

	EAssetType AssetManager::getAssetType(const URL& url)
	{
		std::string extension = g_runtime_context.fileSystem()->extension(url);
		if (m_ext_asset_types.find(extension) != m_ext_asset_types.end())
		{
			return m_ext_asset_types[extension];
		}
		return EAssetType::Invalid;
	}

	VkFilter getVkFilterFromGltf(int gltf_filter)
	{
		switch (gltf_filter)
		{
		case TINYGLTF_TEXTURE_FILTER_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
			return VK_FILTER_NEAREST;
		case INVALID_INDEX:
		case TINYGLTF_TEXTURE_FILTER_LINEAR:
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
			return VK_FILTER_LINEAR;
		}

		return VK_FILTER_NEAREST;
	}

	VkSamplerAddressMode getVkAddressModeFromGltf(int gltf_wrap)
	{
		switch (gltf_wrap)
		{
		case INVALID_INDEX:
		case TINYGLTF_TEXTURE_WRAP_REPEAT:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		}

		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}

	glm::mat4 getGltfNodeMatrix(const tinygltf::Node* node)
	{
		if (!node->matrix.empty()) 
		{
			return glm::make_mat4x4(node->matrix.data());
		};

		glm::mat4 matrix = glm::mat4(1.0f);
		if (!node->translation.empty()) 
		{
			glm::vec3 translation = glm::make_vec3(node->translation.data());
			matrix = glm::translate(matrix, translation);
		}

		if (!node->rotation.empty())
		{
			glm::quat quat = glm::make_quat(node->rotation.data());
			matrix *= glm::mat4_cast(quat);
		}

		if (!node->scale.empty()) 
		{
			glm::vec3 scale = glm::make_vec3(node->scale.data());
			matrix = glm::scale(matrix, scale);
		}
		return matrix;
	}

	bool validateGltfMeshNode(const tinygltf::Node* node, const tinygltf::Model& gltf_model)
	{
		if (node->mesh == INVALID_INDEX)
		{
			return false;
		}
		
		const tinygltf::Mesh& gltf_mesh = gltf_model.meshes[node->mesh];
		for (const tinygltf::Primitive& primitive : gltf_mesh.primitives)
		{
			if (primitive.attributes.find("POSITION") == primitive.attributes.end() ||
				primitive.attributes.find("NORMAL") == primitive.attributes.end() ||
				primitive.attributes.find("TEXCOORD_0") == primitive.attributes.end() ||
				primitive.indices == INVALID_INDEX)
			{
				LOG_WARNING("ignore gltf mesh that doesn't have postion or normal/texcoord/index data");
				return false;
			}
		}

		return true;
	}

	bool isGltfSkeletalMesh(const tinygltf::Mesh& gltf_mesh)
	{
		const tinygltf::Primitive& first_primitive = gltf_mesh.primitives.front();
		return first_primitive.attributes.find("JOINTS_0") != first_primitive.attributes.end() &&
			first_primitive.attributes.find("WEIGHTS_0") != first_primitive.attributes.end();
	}

	size_t findGltfJointNodeBoneIndex(const std::vector<std::pair<tinygltf::Node, int>>& joint_nodes, int node_index)
	{
		for (size_t i = 0; i < joint_nodes.size(); ++i)
		{
			if (joint_nodes[i].second == node_index)
			{
				return i;
			}
		}
		LOG_FATAL("failed to find corresponding joint node index");
		return -1;
	}
	
	uint8_t topologizeGltfBones(std::vector<Bone>& bones, const std::vector<std::pair<tinygltf::Node, int>>& joint_nodes)
	{
		std::vector<size_t> bone_indices;
		for (size_t i = 0; i < joint_nodes.size(); ++i)
		{
			bone_indices.push_back(i);
		}

		for (size_t i = 0; i < joint_nodes.size(); ++i)
		{
			const tinygltf::Node& joint_node = joint_nodes[i].first;
			bones[i].m_name = joint_node.name;
			bones[i].m_local_bind_pose_matrix = getGltfNodeMatrix(&joint_node);

			for (int child_joint_node_index : joint_node.children)
			{
				size_t child_bone_index = findGltfJointNodeBoneIndex(joint_nodes, child_joint_node_index);
				bones[i].m_children.push_back(static_cast<uint8_t>(child_bone_index));
				bones[child_bone_index].m_parent = static_cast<uint8_t>(i);

				bone_indices.erase(std::remove(bone_indices.begin(), bone_indices.end(), child_bone_index), bone_indices.end());
			}
		}

		ASSERT(bone_indices.size() == 1, "failed to find the root bone");
		return static_cast<uint8_t>(bone_indices.front());
	}

	void importGltfTexture(const tinygltf::Model& gltf_model,
		const tinygltf::Image& gltf_image,
		const tinygltf::Sampler& gltf_sampler,
		uint32_t texture_index,
		std::shared_ptr<Texture2D>& texture)
	{
		if (gltf_image.component != IMAGE_COMPONENT)
		{
			LOG_FATAL("unsupported gltf image component: {}", gltf_image.component);
		}
		if (gltf_image.bits != IMAGE_BIT_DEPTH)
		{
			LOG_FATAL("unsupported gltf image bit depth: {}", gltf_image.bits);
		}

		texture->m_width = gltf_image.width;
		texture->m_height = gltf_image.height;
		texture->m_mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(texture->m_width, texture->m_height)))) + 1;
		texture->m_image_data = gltf_image.image;

		texture->m_min_filter = getVkFilterFromGltf(gltf_sampler.minFilter);
		texture->m_mag_filter = getVkFilterFromGltf(gltf_sampler.magFilter);
		texture->m_address_mode_u = getVkAddressModeFromGltf(gltf_sampler.wrapS);
		texture->m_address_mode_v = getVkAddressModeFromGltf(gltf_sampler.wrapT);
		texture->m_address_mode_w = texture->m_address_mode_v;

		// find the texture type according to material that reference it
		for (const tinygltf::Material& gltf_material : gltf_model.materials)
		{
			if (texture_index == gltf_material.pbrMetallicRoughness.baseColorTexture.index)
			{
				texture->m_texture_type = TextureType::BaseColor;
			}
			else if (texture_index == gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.index)
			{
				texture->m_texture_type = TextureType::MetallicRoughness;
			}
			else if (texture_index == gltf_material.normalTexture.index)
			{
				texture->m_texture_type = TextureType::Normal;
			}
			else if (texture_index == gltf_material.occlusionTexture.index)
			{
				texture->m_texture_type = TextureType::Occlusion;
			}
			else if (texture_index == gltf_material.emissiveTexture.index)
			{
				texture->m_texture_type = TextureType::Emissive;
			}
		}
	}

	void importGltfPrimitives(const tinygltf::Model& gltf_model, 
		const std::vector<tinygltf::Primitive> primitives, 
		const std::vector<std::shared_ptr<Material>>& materials,
		const glm::mat4& matrix,
		std::shared_ptr<StaticMesh>& static_mesh,
		std::shared_ptr<SkeletalMesh>& skeletal_mesh)
	{
		glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(matrix)));

		size_t vertex_count = 0, index_count = 0;
		size_t primitive_count = primitives.size();
		for (const tinygltf::Primitive& primitive : primitives)
		{
			vertex_count += gltf_model.accessors[primitive.attributes.find("POSITION")->second].count;
			index_count += gltf_model.accessors[primitive.indices].count;
		}

		std::shared_ptr<Mesh> mesh = nullptr;
		if (static_mesh)
		{
			static_mesh->m_sub_meshes.resize(primitive_count);
			static_mesh->m_vertices.resize(vertex_count);
			static_mesh->m_indices.resize(index_count);
			mesh = static_mesh;
		}
		else if (skeletal_mesh)
		{
			skeletal_mesh->m_sub_meshes.resize(primitive_count);
			skeletal_mesh->m_vertices.resize(vertex_count);
			skeletal_mesh->m_indices.resize(index_count);
			mesh = skeletal_mesh;
		}

		uint32_t vertex_start = 0;
		uint32_t index_start = 0;
		uint32_t index_idx = 0;
		for (int p = 0; p < primitive_count; ++p)
		{
			const tinygltf::Primitive& primitive = primitives[p];

			const tinygltf::Accessor& position_accessor = gltf_model.accessors[primitive.attributes.find("POSITION")->second];
			const tinygltf::BufferView& position_buffer_view = gltf_model.bufferViews[position_accessor.bufferView];
			const float* position_buffer = reinterpret_cast<const float*>(&(gltf_model.buffers[position_buffer_view.buffer].data[
				position_accessor.byteOffset + position_buffer_view.byteOffset]));
			glm::vec3 min_position = glm::vec3(position_accessor.minValues[0], position_accessor.minValues[1], position_accessor.minValues[2]);
			glm::vec3 max_position = glm::vec3(position_accessor.maxValues[0], position_accessor.maxValues[1], position_accessor.maxValues[2]);
			size_t primitive_vertex_count = position_accessor.count;
			int position_byte_stride = position_accessor.ByteStride(position_buffer_view) ? (position_accessor.ByteStride(position_buffer_view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

			const tinygltf::Accessor& tex_coord_accessor = gltf_model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
			const tinygltf::BufferView& tex_coord_buffer_view = gltf_model.bufferViews[tex_coord_accessor.bufferView];
			const float* tex_coord_buffer = reinterpret_cast<const float*>(&(gltf_model.buffers[tex_coord_buffer_view.buffer].data[tex_coord_accessor.byteOffset + tex_coord_buffer_view.byteOffset]));
			int tex_coord_byte_stride = tex_coord_accessor.ByteStride(tex_coord_buffer_view) ? (tex_coord_accessor.ByteStride(tex_coord_buffer_view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);

			const tinygltf::Accessor& normal_accessor = gltf_model.accessors[primitive.attributes.find("NORMAL")->second];
			const tinygltf::BufferView& normal_buffer_view = gltf_model.bufferViews[normal_accessor.bufferView];
			const float* normal_buffer = reinterpret_cast<const float*>(&(gltf_model.buffers[normal_buffer_view.buffer].data[normal_accessor.byteOffset + normal_buffer_view.byteOffset]));
			int normal_byte_stride = normal_accessor.ByteStride(normal_buffer_view) ? (normal_accessor.ByteStride(normal_buffer_view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

			const void* joint_void_buffer = nullptr;
			const float* weight_buffer = nullptr;
			int joint_component_type = INVALID_INDEX;
			int joint_byte_stride = INVALID_INDEX;
			int weight_byte_stride = INVALID_INDEX;
			if (!static_mesh)
			{
				const tinygltf::Accessor& joint_accessor = gltf_model.accessors[primitive.attributes.find("JOINTS_0")->second];
				const tinygltf::BufferView& joint_buffer_view = gltf_model.bufferViews[joint_accessor.bufferView];
				joint_void_buffer = &(gltf_model.buffers[joint_buffer_view.buffer].data[joint_accessor.byteOffset + joint_buffer_view.byteOffset]);
				joint_component_type = joint_accessor.componentType;
				joint_byte_stride = joint_accessor.ByteStride(joint_buffer_view) ? (joint_accessor.ByteStride(joint_buffer_view) / tinygltf::GetComponentSizeInBytes(joint_component_type)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);

				const tinygltf::Accessor& weight_accessor = gltf_model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
				const tinygltf::BufferView& weight_buffer_view = gltf_model.bufferViews[weight_accessor.bufferView];
				weight_buffer = reinterpret_cast<const float*>(&(gltf_model.buffers[weight_buffer_view.buffer].data[weight_accessor.byteOffset + weight_buffer_view.byteOffset]));
				weight_byte_stride = weight_accessor.ByteStride(weight_buffer_view) ? (weight_accessor.ByteStride(weight_buffer_view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
			}

			// set vertices
			for (size_t v = 0; v < primitive_vertex_count; ++v)
			{
				StaticVertex* static_vertex = nullptr;
				SkeletalVertex* skeletal_vertex = nullptr;
				if (static_mesh)
				{
					static_vertex = &static_mesh->m_vertices[v + vertex_start];
				}
				else
				{
					skeletal_vertex = &skeletal_mesh->m_vertices[v + vertex_start];
					static_vertex = skeletal_vertex;
				}

				static_vertex->m_position = glm::make_vec3(&position_buffer[v * position_byte_stride]);
				static_vertex->m_position = matrix * glm::vec4(static_vertex->m_position, 1.0f);
				static_vertex->m_tex_coord = glm::make_vec2(&tex_coord_buffer[v * tex_coord_byte_stride]);
				static_vertex->m_normal = glm::make_vec3(&normal_buffer[v * normal_byte_stride]);
				static_vertex->m_normal = normal_matrix * static_vertex->m_normal;

				if (!static_mesh)
				{
					switch (joint_component_type)
					{
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
					{
						const uint16_t* joint_buffer = static_cast<const uint16_t*>(joint_void_buffer);
						skeletal_vertex->m_bones = glm::vec4(glm::make_vec4(&joint_buffer[v * joint_byte_stride]));
						break;
					}
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
					{
						const uint8_t* joint_buffer = static_cast<const uint8_t*>(joint_void_buffer);
						skeletal_vertex->m_bones = glm::vec4(glm::make_vec4(&joint_buffer[v * joint_byte_stride]));
						break;
					}
					default:
						LOG_FATAL("unknow gltf mesh joint component type");
						break;
					}

					skeletal_vertex->m_weights = glm::make_vec4(&weight_buffer[v * weight_byte_stride]);
					if (glm::length(skeletal_vertex->m_weights) < k_epsilon)
					{
						skeletal_vertex->m_weights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
					}
				}
			}

			// set indices
			const tinygltf::Accessor& index_accessor = gltf_model.accessors[primitive.indices];
			const tinygltf::BufferView& index_buffer_view = gltf_model.bufferViews[index_accessor.bufferView];
			const void* index_void_buffer = &(gltf_model.buffers[index_buffer_view.buffer].data[index_accessor.byteOffset + index_buffer_view.byteOffset]);
			size_t primitive_index_count = static_cast<uint32_t>(index_accessor.count);

			switch (index_accessor.componentType)
			{
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
			{
				const uint32_t* index_buffer = static_cast<const uint32_t*>(index_void_buffer);
				for (size_t i = 0; i < primitive_index_count; ++i)
				{
					mesh->m_indices[index_idx++] = index_buffer[i] + vertex_start;
				}
				break;
			}
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
			{
				const uint16_t* index_buffer = static_cast<const uint16_t*>(index_void_buffer);
				for (size_t i = 0; i < primitive_index_count; ++i)
				{
					mesh->m_indices[index_idx++] = index_buffer[i] + vertex_start;
				}
				break;
			}
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
			{
				const uint8_t* index_buffer = static_cast<const uint8_t*>(index_void_buffer);
				for (size_t i = 0; i < primitive_index_count; ++i)
				{
					mesh->m_indices[index_idx++] = index_buffer[i] + vertex_start;
				}
				break;
			}
			default:
				LOG_FATAL("unknow gltf mesh index component type");
				break;
			}

			// set submesh
			SubMesh* sub_mesh = &mesh->m_sub_meshes[p];
			sub_mesh->m_index_offset = index_start;
			sub_mesh->m_index_count = primitive_index_count;
			sub_mesh->m_vertex_count = primitive_vertex_count;
			sub_mesh->m_bounding_box = BoundingBox{ min_position, max_position };
			sub_mesh->m_bounding_box.transform(matrix);
			REFERENCE_ASSET(sub_mesh, m_material, materials[primitive.material]);

			vertex_start += primitive_vertex_count;
			index_start += primitive_index_count;
		}
	}

	bool AssetManager::importGltf(const std::string& filename, const URL& folder, bool is_combined)
	{
		tinygltf::Model gltf_model;
		tinygltf::TinyGLTF gltf_context;

		std::string error;
		std::string warning;

		std::string basename = g_runtime_context.fileSystem()->basename(filename);
		std::string extension = g_runtime_context.fileSystem()->extension(filename);
		bool is_binary = extension == "glb";
		bool success = false;
		if (is_binary)
		{
			success = gltf_context.LoadBinaryFromFile(&gltf_model, &error, &warning, filename);
		}
		else
		{
			success = gltf_context.LoadASCIIFromFile(&gltf_model, &error, &warning, filename);
		}

		if (!success)
		{
			LOG_FATAL("failed to load gltf file {}! error: {}, warning: {}", filename, error, warning);
			return false;
		}

		if (!error.empty())
		{
			LOG_ERROR("load gltf file {} error: {}", filename, error);
		}
		if (!warning.empty())
		{
			LOG_WARNING("load gltf file {} warning: {}", filename, warning);
		}

		std::map<EAssetType, int> asset_indices = {
			{ EAssetType::Texture2D, 0},
			{ EAssetType::TextureCube, 0},
			{ EAssetType::Material, 0},
			{ EAssetType::Skeleton, 0},
			{ EAssetType::StaticMesh, 0},
			{ EAssetType::SkeletalMesh, 0},
			{ EAssetType::Animation, 0}
		};

		// 1.load textures and samplers
		std::vector<std::shared_ptr<Texture2D>> textures;
		for (const tinygltf::Texture& gltf_texture : gltf_model.textures)
		{
			const tinygltf::Image& gltf_image = gltf_model.images[gltf_texture.source];
			const tinygltf::Sampler& gltf_sampler = gltf_model.samplers[gltf_texture.sampler];

			EAssetType asset_type = EAssetType::Texture2D;
			std::string asset_name = getAssetName(basename, gltf_texture.name, asset_type, asset_indices[asset_type]++);
			URL url = g_runtime_context.fileSystem()->combine(folder, asset_name);
			std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>();
			texture->setURL(url);
			importGltfTexture(gltf_model, gltf_image, gltf_sampler, static_cast<uint32_t>(textures.size()), texture);

			texture->inflate();
			serializeAsset(texture);
			textures.push_back(texture);
			m_assets[url] = texture;
		}

		// 2.load materials
		std::vector<std::shared_ptr<Material>> materials;
		for (const tinygltf::Material& gltf_material : gltf_model.materials)
		{
			EAssetType asset_type = EAssetType::Material;
			std::string asset_name = getAssetName(basename, gltf_material.name, asset_type, asset_indices[asset_type]++);
			URL url = g_runtime_context.fileSystem()->combine(folder, asset_name);

			std::shared_ptr<Material> material = std::make_shared<Material>();
			material->setURL(url);

			material->m_base_color_factor = glm::make_vec4(gltf_material.pbrMetallicRoughness.baseColorFactor.data());
			material->m_emissive_factor = glm::make_vec4(gltf_material.emissiveFactor.data());
			material->m_metallic_factor = gltf_material.pbrMetallicRoughness.metallicFactor;
			material->m_roughness_factor = gltf_material.pbrMetallicRoughness.roughnessFactor;

			if (gltf_material.pbrMetallicRoughness.baseColorTexture.index != INVALID_INDEX)
			{
				ASSERT(gltf_material.pbrMetallicRoughness.baseColorTexture.texCoord == 0, "do not support non-zero texture coordinate index");
				REFERENCE_ASSET(material, m_base_color_texure, textures[gltf_material.pbrMetallicRoughness.baseColorTexture.index]);
			}
			if (gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.index != INVALID_INDEX)
			{
				ASSERT(gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord == 0, "do not support non-zero texture coordinate index");
				REFERENCE_ASSET(material, m_metallic_roughness_texure, textures[gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.index]);
			}
			if (gltf_material.normalTexture.index != INVALID_INDEX)
			{
				ASSERT(gltf_material.normalTexture.texCoord == 0, "do not support non-zero texture coordinate index");
				REFERENCE_ASSET(material, m_normal_texure, textures[gltf_material.normalTexture.index]);
			}
			if (gltf_material.occlusionTexture.index != INVALID_INDEX)
			{
				ASSERT(gltf_material.occlusionTexture.texCoord == 0, "do not support non-zero texture coordinate index");
				REFERENCE_ASSET(material, m_occlusion_texure, textures[gltf_material.occlusionTexture.index]);
			}
			if (gltf_material.emissiveTexture.index != INVALID_INDEX)
			{
				ASSERT(gltf_material.emissiveTexture.texCoord == 0, "do not support non-zero texture coordinate index");
				REFERENCE_ASSET(material, m_emissive_texure, textures[gltf_material.emissiveTexture.index]);
			}

			serializeAsset(material);
			materials.push_back(material);
			m_assets[url] = material;
		}

		// 3.load nodes recursively
		// load all nodes into one single vector, with global world matrix
		std::vector<std::pair<glm::mat4, const tinygltf::Node*>> nodes;
		for (const tinygltf::Scene& gltf_scene : gltf_model.scenes)
		{
			std::queue<std::pair<glm::mat4, const tinygltf::Node*>> node_queue;
			for (int index : gltf_scene.nodes)
			{
				node_queue.push(std::make_pair(glm::mat4(1.0f), &gltf_model.nodes[index]));
			}

			while (!node_queue.empty())
			{
				std::pair<glm::mat4, const tinygltf::Node*> node_pair = node_queue.front();
				node_queue.pop();

				const glm::mat4& parent_matrix = node_pair.first;
				const tinygltf::Node* parent_node = node_pair.second;
				if (validateGltfMeshNode(parent_node, gltf_model))
				{
					nodes.push_back(node_pair);
				}
				
				for (int children : parent_node->children)
				{
					const tinygltf::Node* children_node = &gltf_model.nodes[children];
					glm::mat4 children_matrix = parent_matrix * getGltfNodeMatrix(children_node);
					node_queue.push(std::make_pair(children_matrix, children_node));
				}
			}
		}

		// turn nodes to static mesh/skeletal mesh
		if (is_combined)
		{
			EAssetType asset_type = EAssetType::StaticMesh;
			std::string asset_name = getAssetName(basename, basename, asset_type, asset_indices[asset_type]++);
			URL url = g_runtime_context.fileSystem()->combine(folder, asset_name);
			std::shared_ptr<StaticMesh> static_mesh = std::make_shared<StaticMesh>();
			static_mesh->setURL(url);
			std::shared_ptr<SkeletalMesh> skeletal_mesh = nullptr;

			std::vector<tinygltf::Primitive> primitives;
			for (const auto& node_pair : nodes)
			{
				const tinygltf::Mesh& gltf_mesh = gltf_model.meshes[node_pair.second->mesh];
				primitives.insert(primitives.end(), gltf_mesh.primitives.begin(), gltf_mesh.primitives.end());
			}

			importGltfPrimitives(gltf_model, primitives, materials, glm::mat4(1.0f), static_mesh, skeletal_mesh);

			static_mesh->inflate();
			serializeAsset(static_mesh);
			m_assets[url] = static_mesh;
		}
		else
		{
			for (const auto& node_pair : nodes)
			{
				const tinygltf::Mesh& gltf_mesh = gltf_model.meshes[node_pair.second->mesh];
				bool is_skeletal_mesh = isGltfSkeletalMesh(gltf_mesh);
				EAssetType asset_type = is_skeletal_mesh ? EAssetType::SkeletalMesh : EAssetType::StaticMesh;
				std::string asset_name = getAssetName(basename, gltf_mesh.name, asset_type, asset_indices[asset_type]++);
				URL url = g_runtime_context.fileSystem()->combine(folder, asset_name);

				std::shared_ptr<StaticMesh> static_mesh = nullptr;
				std::shared_ptr<SkeletalMesh> skeletal_mesh = nullptr;
				if (is_skeletal_mesh)
				{
					skeletal_mesh = std::make_shared<SkeletalMesh>();
					skeletal_mesh->setURL(url);
				}
				else
				{
					static_mesh = std::make_shared<StaticMesh>();
					static_mesh->setURL(url);
				}

				importGltfPrimitives(gltf_model, gltf_mesh.primitives, materials, node_pair.first, static_mesh, skeletal_mesh);

				if (is_skeletal_mesh)
				{
					skeletal_mesh->inflate();
					serializeAsset(skeletal_mesh);
					m_assets[url] = skeletal_mesh;
				}
				else
				{
					static_mesh->inflate();
					serializeAsset(static_mesh);
					m_assets[url] = static_mesh;
				}
			}
		}

		// load skeletons
		for (const tinygltf::Skin& skin : gltf_model.skins)
		{
			EAssetType asset_type = EAssetType::Skeleton;
			std::string asset_name = getAssetName(basename, skin.name, asset_type, asset_indices[asset_type]++);
			URL url = g_runtime_context.fileSystem()->combine(folder, asset_name);

			std::shared_ptr<Skeleton> skeleton = std::make_shared<Skeleton>();
			skeleton->setURL(url);
			skeleton->m_name = skin.name;

			uint32_t joint_count = static_cast<uint32_t>(skin.joints.size());
			skeleton->m_bones.resize(joint_count);

			std::vector<std::pair<tinygltf::Node, int>> joint_nodes(joint_count);
			for (size_t i = 0; i < joint_count; ++i)
			{
				int joint_index = skin.joints[i];
				joint_nodes[i] = std::make_pair(gltf_model.nodes[joint_index], joint_index);
			}

			// set bone's children/parent relations
			skeleton->m_root_bone_index = topologizeGltfBones(skeleton->m_bones, joint_nodes);

			// set bone's global inverse bind matrix
			ASSERT(skin.inverseBindMatrices != INVALID_INDEX, "gltf skin must have inverse bind matrices");
			const tinygltf::Accessor& accessor = gltf_model.accessors[skin.inverseBindMatrices];
			const tinygltf::BufferView& buffer_view = gltf_model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = gltf_model.buffers[buffer_view.buffer];
			std::vector<glm::mat4> inverse_bind_matrices(accessor.count);
			memcpy(inverse_bind_matrices.data(), &buffer.data[accessor.byteOffset + buffer_view.byteOffset], accessor.count * sizeof(glm::mat4));
			for (size_t i = 0; i < skeleton->m_bones.size(); ++i)
			{
				skeleton->m_bones[i].m_global_inverse_bind_pose_matrix = inverse_bind_matrices[i];
			}

			skeleton->inflate();
			serializeAsset(skeleton);
			m_assets[url] = skeleton;
		}

		// load animations
		for (const tinygltf::Animation& gltf_animation : gltf_model.animations)
		{
			EAssetType asset_type = EAssetType::Animation;
			std::string asset_name = getAssetName(basename, gltf_animation.name, asset_type, asset_indices[asset_type]++);
			URL url = g_runtime_context.fileSystem()->combine(folder, asset_name);

			std::shared_ptr<Animation> animation = std::make_shared<Animation>();
			animation->setURL(url);
			animation->m_name = gltf_animation.name;

			// get animation samplers
			animation->m_samplers.resize(gltf_animation.samplers.size());
			for (size_t i = 0; i < gltf_animation.samplers.size(); ++i)
			{
				const tinygltf::AnimationSampler& gltf_sampler = gltf_animation.samplers[i];
				AnimationSampler& sampler = animation->m_samplers[i];

				sampler.m_interp_type = gltf_sampler.interpolation == "LINEAR" ? AnimationSampler::EInterpolationType::Linear :
					(gltf_sampler.interpolation == "STEP" ? AnimationSampler::EInterpolationType::Step : AnimationSampler::EInterpolationType::CubicSpline);
				
				// get sampler times
				{
					const tinygltf::Accessor& accessor = gltf_model.accessors[gltf_sampler.input];
					const tinygltf::BufferView& buffer_view = gltf_model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = gltf_model.buffers[buffer_view.buffer];
					ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "gltf animation sampler's input component type must be float");

					sampler.m_times.resize(accessor.count);
					memcpy(sampler.m_times.data(), &buffer.data[accessor.byteOffset + buffer_view.byteOffset], accessor.count * sizeof(float));
				}
				
				// get sampler values
				{
					const tinygltf::Accessor& accessor = gltf_model.accessors[gltf_sampler.output];
					const tinygltf::BufferView& buffer_view = gltf_model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = gltf_model.buffers[buffer_view.buffer];
					ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "gltf animation sampler's output component type must be float");
				
					sampler.m_values.resize(accessor.count);
					switch (accessor.type) 
					{
					case TINYGLTF_TYPE_VEC3: 
					{
						const void* data_ptr = &buffer.data[accessor.byteOffset + buffer_view.byteOffset];
						const glm::vec3* buffer = static_cast<const glm::vec3*>(data_ptr);
						for (size_t i = 0; i < accessor.count; i++) 
						{
							sampler.m_values[i] = glm::vec4(buffer[i], 0.0f);
						}
						break;
					}
					case TINYGLTF_TYPE_VEC4: 
					{
						memcpy(sampler.m_values.data(), &buffer.data[accessor.byteOffset + buffer_view.byteOffset], accessor.count * sizeof(glm::vec4));
						break;
					}
					default: 
					{
						LOG_FATAL("unknown gltf animation sampler's value type");
						break;
					}
					}
				}
			}

			// get animation channels
			animation->m_channels.resize(gltf_animation.channels.size());
			for (size_t i = 0; i < gltf_animation.channels.size(); ++i)
			{
				const tinygltf::AnimationChannel& gltf_channel = gltf_animation.channels[i];
				AnimationChannel& channel = animation->m_channels[i];
				ASSERT(gltf_channel.target_path == "rotation" || gltf_channel.target_path == "translation" || gltf_channel.target_path == "scale", 
					"gltf animation channel's target path must be rotation/translation/scale");

				channel.m_path_type = gltf_channel.target_path == "rotation" ? AnimationChannel::EPathType::Rotation :
					(gltf_channel.target_path == "translation" ? AnimationChannel::EPathType::Translation : AnimationChannel::EPathType::Scale);
				channel.m_bone_name = gltf_model.nodes[gltf_channel.target_node].name;
				channel.m_sampler_index = gltf_channel.sampler;
			}

			animation->inflate();
			serializeAsset(animation);
			m_assets[url] = animation;
		}

		return true;
	}

	void AssetManager::serializeAsset(std::shared_ptr<Asset> asset)
	{
		EAssetType asset_type = asset->getAssetType();
		const std::string& asset_ext = m_asset_type_exts[asset_type];
		EArchiveType archive_type = m_asset_archive_types[asset_type];
		std::string filename = TO_ABSOLUTE(asset->getURL());

		switch (archive_type)
		{
		case EArchiveType::Json:
		{
			std::ofstream ofs(filename);
			cereal::JSONOutputArchive archive(ofs);
			archive(cereal::make_nvp(asset_ext.c_str(), asset));
		}
		break;
		case EArchiveType::Binary:
		{
			std::ofstream ofs(filename, std::ios::binary);
			cereal::BinaryOutputArchive archive(ofs);
			archive(cereal::make_nvp(asset_ext.c_str(), asset));
		}
		break;
		default:
			break;
		}
	}

	std::shared_ptr<Asset> AssetManager::deserializeAsset(const URL& url)
	{
		// check if the asset url exists
		if (!g_runtime_context.fileSystem()->exists(url))
		{
			return nullptr;
		}

		// check if the asset has been loaded
		if (m_assets.find(url) != m_assets.end())
		{
			return m_assets[url];
		}

		EAssetType asset_type = getAssetType(url);
		EArchiveType archive_type = m_asset_archive_types[asset_type];
		const std::string& asset_ext = m_asset_type_exts[asset_type];
		std::string filename = TO_ABSOLUTE(url);
		std::shared_ptr<Asset> asset = nullptr;

		switch (archive_type)
		{
		case EArchiveType::Json:
		{
			std::ifstream ifs(filename);
			cereal::JSONInputArchive archive(ifs);
			archive(asset);
		}
		break;
		case EArchiveType::Binary:
		{
			std::ifstream ifs(filename, std::ios::binary);
			cereal::BinaryInputArchive archive(ifs);
			archive(asset);
		}
		break;
		default:
			break;
		}

		asset->setURL(url);
		asset->inflate();
		m_assets[url] = asset;

		return asset;
	}

	std::string AssetManager::getAssetName(const std::string& basename, const std::string& asset_name, EAssetType asset_type, int asset_index)
	{
		const std::string& ext = m_asset_type_exts[asset_type];
		if (!asset_name.empty())
		{
			std::string asset_basename = g_runtime_context.fileSystem()->basename(asset_name);
			return g_runtime_context.fileSystem()->format("%s_%s.%s", ext.c_str(), asset_basename.c_str(), ext.c_str());
		}
		return g_runtime_context.fileSystem()->format("%s_%s_%d.%s", ext.c_str(), basename.c_str(), asset_index, ext.c_str());
	}
}