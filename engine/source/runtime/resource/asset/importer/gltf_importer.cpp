#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "gltf_importer.h"

#include "runtime/core/base/macro.h"
#include "runtime/resource/asset/asset_manager.h"

#include <queue>
#include <algorithm>

namespace Bamboo
{
	VkFilter GltfImporter::getVkFilterFromGltf(int gltf_filter)
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

	VkSamplerAddressMode GltfImporter::getVkAddressModeFromGltf(int gltf_wrap)
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

	glm::mat4 GltfImporter::getGltfNodeMatrix(const tinygltf::Node* node)
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

	QTranform GltfImporter::getGltfNodeTransform(const tinygltf::Node* node)
	{
		QTranform transform;
		if (!node->translation.empty())
		{
			transform.m_position = glm::make_vec3(node->translation.data());
		}
		if (!node->rotation.empty())
		{
			transform.m_rotation = glm::make_quat(node->rotation.data());
		}
		if (!node->scale.empty())
		{
			transform.m_scale = glm::make_vec3(node->scale.data());
		}
		return transform;
	}

	bool GltfImporter::validateGltfMeshNode(const tinygltf::Node* node, const tinygltf::Model& gltf_model)
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
				//primitive.attributes.find("TEXCOORD_0") == primitive.attributes.end() ||
				primitive.indices == INVALID_INDEX)
			{
				LOG_WARNING("ignore gltf mesh that doesn't have postion or normal/texcoord/index data");
				return false;
			}
		}

		return true;
	}

	bool GltfImporter::isGltfSkeletalMesh(const tinygltf::Mesh& gltf_mesh)
	{
		const tinygltf::Primitive& first_primitive = gltf_mesh.primitives.front();
		return first_primitive.attributes.find("JOINTS_0") != first_primitive.attributes.end() &&
			first_primitive.attributes.find("WEIGHTS_0") != first_primitive.attributes.end();
	}

	size_t GltfImporter::findGltfJointNodeBoneIndex(const std::vector<std::pair<tinygltf::Node, int>>& joint_nodes, int node_index)
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

	uint8_t GltfImporter::topologizeGltfBones(std::vector<Bone>& bones, const std::vector<std::pair<tinygltf::Node, int>>& joint_nodes)
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
			bones[i].m_local_bind_pose_transform = getGltfNodeTransform(&joint_node);

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

	void GltfImporter::importGltfTexture(const tinygltf::Model& gltf_model,
		const tinygltf::Image& gltf_image,
		const tinygltf::Sampler& gltf_sampler,
		uint32_t texture_index,
		std::shared_ptr<Texture2D>& texture)
	{
		texture->m_width = gltf_image.width;
		texture->m_height = gltf_image.height;
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
				texture->m_texture_type = ETextureType::BaseColor;
			}
			else if (texture_index == gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.index)
			{
				texture->m_texture_type = ETextureType::MetallicRoughnessOcclusion;
			}
			else if (texture_index == gltf_material.normalTexture.index)
			{
				texture->m_texture_type = ETextureType::Normal;
			}
			else if (texture_index == gltf_material.emissiveTexture.index)
			{
				texture->m_texture_type = ETextureType::Emissive;
			}
		}

		// set texture pixel type
		const static std::map<std::pair<int, int>, EPixelType> component_bits_pixel_type_map = {
			{ std::make_pair(4, 8), EPixelType::RGBA8 },
			{ std::make_pair(4, 16), EPixelType::RGBA16 },
			{ std::make_pair(4, 32), EPixelType::RGBA32 },
			{ std::make_pair(2, 16), EPixelType::RG16 },
			{ std::make_pair(1, 16), EPixelType::R16 },
			{ std::make_pair(1, 32), EPixelType::R32 },
		};

		std::pair<int, int> component_bits = std::make_pair(gltf_image.component, gltf_image.bits);
		if (component_bits_pixel_type_map.find(component_bits) != component_bits_pixel_type_map.end())
		{
			texture->m_pixel_type = component_bits_pixel_type_map.at(component_bits);
		}
		else
		{
			LOG_FATAL("unsupported image component/bits pair:({}, {})", component_bits.first, component_bits.second);
		}
	}

	void GltfImporter::importGltfPrimitives(const tinygltf::Model& gltf_model,
		const std::vector<std::pair<tinygltf::Primitive, glm::mat4>>& primitives,
		const std::vector<std::shared_ptr<Material>>& materials,
		std::shared_ptr<StaticMesh>& static_mesh,
		std::shared_ptr<SkeletalMesh>& skeletal_mesh)
	{
		size_t vertex_count = 0, index_count = 0;
		size_t primitive_count = primitives.size();
		for (const auto& primitive_pair : primitives)
		{
			vertex_count += gltf_model.accessors[primitive_pair.first.attributes.find("POSITION")->second].count;
			index_count += gltf_model.accessors[primitive_pair.first.indices].count;
		}

		std::shared_ptr<Mesh> mesh = nullptr;
		std::shared_ptr<Asset> mesh_asset = nullptr;
		if (static_mesh)
		{
			static_mesh->m_sub_meshes.resize(primitive_count);
			static_mesh->m_vertices.resize(vertex_count);
			static_mesh->m_indices.resize(index_count);
			mesh = static_mesh;
			mesh_asset = static_mesh;
		}
		else if (skeletal_mesh)
		{
			skeletal_mesh->m_sub_meshes.resize(primitive_count);
			skeletal_mesh->m_vertices.resize(vertex_count);
			skeletal_mesh->m_indices.resize(index_count);
			mesh = skeletal_mesh;
			mesh_asset = skeletal_mesh;
		}

		std::string folder = mesh_asset->getFolder();
		std::string bare_name = mesh_asset->getBareName();
		const auto& as = g_runtime_context.assetManager();

		uint32_t vertex_start = 0;
		uint32_t index_start = 0;
		uint32_t index_idx = 0;
		uint32_t dummy_material_index = 0;
		for (int p = 0; p < primitive_count; ++p)
		{
			const tinygltf::Primitive& primitive = primitives[p].first;

			const tinygltf::Accessor& position_accessor = gltf_model.accessors[primitive.attributes.find("POSITION")->second];
			const tinygltf::BufferView& position_buffer_view = gltf_model.bufferViews[position_accessor.bufferView];
			const float* position_buffer = reinterpret_cast<const float*>(&(gltf_model.buffers[position_buffer_view.buffer].data[
				position_accessor.byteOffset + position_buffer_view.byteOffset]));
			glm::vec3 min_position = glm::vec3(position_accessor.minValues[0], position_accessor.minValues[1], position_accessor.minValues[2]);
			glm::vec3 max_position = glm::vec3(position_accessor.maxValues[0], position_accessor.maxValues[1], position_accessor.maxValues[2]);
			size_t primitive_vertex_count = position_accessor.count;
			int position_byte_stride = position_accessor.ByteStride(position_buffer_view) ? (position_accessor.ByteStride(position_buffer_view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

			const float* tex_coord_buffer = nullptr;
			int tex_coord_byte_stride = 0;
			if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
			{
				const tinygltf::Accessor& tex_coord_accessor = gltf_model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
				const tinygltf::BufferView& tex_coord_buffer_view = gltf_model.bufferViews[tex_coord_accessor.bufferView];
				tex_coord_buffer = reinterpret_cast<const float*>(&(gltf_model.buffers[tex_coord_buffer_view.buffer].data[tex_coord_accessor.byteOffset + tex_coord_buffer_view.byteOffset]));
				tex_coord_byte_stride = tex_coord_accessor.ByteStride(tex_coord_buffer_view) ? (tex_coord_accessor.ByteStride(tex_coord_buffer_view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
			}
			
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
			const glm::mat4& matrix = primitives[p].second;
			glm::mat3 normal_matrix = glm::mat3(glm::transpose(glm::inverse(matrix)));
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
				static_vertex->m_tex_coord = tex_coord_buffer ? glm::make_vec2(&tex_coord_buffer[v * tex_coord_byte_stride]) : glm::vec2(0.0f);
				static_vertex->m_normal = glm::make_vec3(&normal_buffer[v * normal_byte_stride]);

				if (static_mesh)
				{
 					static_vertex->m_position = matrix * glm::vec4(static_vertex->m_position, 1.0f);
 					static_vertex->m_normal = glm::normalize(normal_matrix * static_vertex->m_normal);
				}
				else
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

			// if submesh doesn't have material, use default material
			std::shared_ptr<Material> material;
			if (primitive.material == INVALID_INDEX)
			{
				material = as->loadAsset<Material>(DEFAULT_MATERIAL_URL);
			}
			else
			{
				material = materials[primitive.material];
			}
			REF_ASSET_OUTER(sub_mesh, m_material, material)

			vertex_start += primitive_vertex_count;
			index_start += primitive_index_count;
		}
	}

	bool GltfImporter::importGltf(const std::string& filename, const URL& folder, const GltfImportOption& option)
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

		// get global asset manager
		const auto& as = g_runtime_context.assetManager();

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
			static tinygltf::Sampler default_sampler{};
			const tinygltf::Sampler& gltf_sampler = (!gltf_model.samplers.empty() && gltf_texture.sampler > INVALID_INDEX) ?
				gltf_model.samplers[gltf_texture.sampler] : default_sampler;

			EAssetType asset_type = EAssetType::Texture2D;
			std::string asset_name = as->getAssetName(gltf_texture.name, asset_type, asset_indices[asset_type]++, basename);
			URL url = g_runtime_context.fileSystem()->combine(folder, asset_name);
			std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>();
			texture->setURL(url);
			importGltfTexture(gltf_model, gltf_image, gltf_sampler, static_cast<uint32_t>(textures.size()), texture);

			texture->inflate();
			as->serializeAsset(texture);
			textures.push_back(texture);
		}

		// 2.load materials
		std::vector<std::shared_ptr<Material>> materials;
		for (const tinygltf::Material& gltf_material : gltf_model.materials)
		{
			EAssetType asset_type = EAssetType::Material;
			std::string asset_name = as->getAssetName(gltf_material.name, asset_type, asset_indices[asset_type]++, basename);
			URL url = g_runtime_context.fileSystem()->combine(folder, asset_name);

			std::shared_ptr<Material> material = std::make_shared<Material>();
			material->setURL(url);

			material->m_base_color_factor = glm::make_vec4(gltf_material.pbrMetallicRoughness.baseColorFactor.data());
			material->m_emissive_factor = glm::make_vec4(gltf_material.emissiveFactor.data());
			material->m_metallic_factor = gltf_material.pbrMetallicRoughness.metallicFactor;
			material->m_roughness_factor = gltf_material.pbrMetallicRoughness.roughnessFactor;
			material->m_contains_occlusion_channel = option.contains_occlusion_channel;

			if (gltf_material.pbrMetallicRoughness.baseColorTexture.index != INVALID_INDEX)
			{
				ASSERT(gltf_material.pbrMetallicRoughness.baseColorTexture.texCoord == 0, "do not support non-zero texture coordinate index");
				REF_ASSET_OUTER(material, m_base_color_texure, textures[gltf_material.pbrMetallicRoughness.baseColorTexture.index])
			}
			if (gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.index != INVALID_INDEX)
			{
				ASSERT(gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord == 0, "do not support non-zero texture coordinate index");
				REF_ASSET_OUTER(material, m_metallic_roughness_occlusion_texure, textures[gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.index])
			}
			if (gltf_material.normalTexture.index != INVALID_INDEX)
			{
				ASSERT(gltf_material.normalTexture.texCoord == 0, "do not support non-zero texture coordinate index");
				REF_ASSET_OUTER(material, m_normal_texure, textures[gltf_material.normalTexture.index])
			}
			if (gltf_material.emissiveTexture.index != INVALID_INDEX)
			{
				ASSERT(gltf_material.emissiveTexture.texCoord == 0, "do not support non-zero texture coordinate index");
				REF_ASSET_OUTER(material, m_emissive_texure, textures[gltf_material.emissiveTexture.index])
			}

			as->serializeAsset(material);
			materials.push_back(material);
		}

		// 3.load nodes recursively
		// load all nodes into one single vector, with global world matrix
		glm::mat4 root_bone_matrix = glm::mat4(1.0);
		std::vector<std::pair<glm::mat4, const tinygltf::Node*>> nodes;
		for (const tinygltf::Scene& gltf_scene : gltf_model.scenes)
		{
			std::queue<std::pair<glm::mat4, const tinygltf::Node*>> node_queue;
			for (int index : gltf_scene.nodes)
			{
				const tinygltf::Node* node = &gltf_model.nodes[index];
				node_queue.push(std::make_pair(getGltfNodeMatrix(node), node));
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

				if (parent_node->mesh != INVALID_INDEX && parent_node->skin != INVALID_INDEX)
				{
					root_bone_matrix = parent_matrix;
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
		if (option.combine_meshes)
		{
			// determine mesh type by skins
			bool is_skeletal_mesh = !option.force_static_mesh && !gltf_model.skins.empty();
			EAssetType asset_type = is_skeletal_mesh ? EAssetType::SkeletalMesh : EAssetType::StaticMesh;
			std::string asset_name = as->getAssetName(basename, asset_type, asset_indices[asset_type]++, basename);
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

			std::vector<std::pair<tinygltf::Primitive, glm::mat4>> primitives;
			for (const auto& node_pair : nodes)
			{
				const tinygltf::Mesh& gltf_mesh = gltf_model.meshes[node_pair.second->mesh];
				for (const tinygltf::Primitive& primitive : gltf_mesh.primitives)
				{
					primitives.push_back(std::make_pair(primitive, node_pair.first));
				}
			}

			importGltfPrimitives(gltf_model, primitives, materials, static_mesh, skeletal_mesh);

			if (is_skeletal_mesh)
			{
				skeletal_mesh->inflate();
				as->serializeAsset(skeletal_mesh);
			}
			else
			{
				static_mesh->inflate();
				as->serializeAsset(static_mesh);
			}
		}
		else
		{
			for (const auto& node_pair : nodes)
			{
				const tinygltf::Mesh& gltf_mesh = gltf_model.meshes[node_pair.second->mesh];
				bool is_skeletal_mesh = option.force_static_mesh ? false : isGltfSkeletalMesh(gltf_mesh);
				EAssetType asset_type = is_skeletal_mesh ? EAssetType::SkeletalMesh : EAssetType::StaticMesh;
				std::string asset_name = as->getAssetName(gltf_mesh.name, asset_type, asset_indices[asset_type]++, basename);
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

				std::vector<std::pair<tinygltf::Primitive, glm::mat4>> primitives;
				for (const tinygltf::Primitive& primitive : gltf_mesh.primitives)
				{
					primitives.push_back(std::make_pair(primitive, node_pair.first));
				}
				importGltfPrimitives(gltf_model, primitives, materials, static_mesh, skeletal_mesh);

				if (is_skeletal_mesh)
				{
					skeletal_mesh->inflate();
					as->serializeAsset(skeletal_mesh);
				}
				else
				{
					static_mesh->inflate();
					as->serializeAsset(static_mesh);
				}
			}
		}

		// load skeletons
		for (const tinygltf::Skin& skin : gltf_model.skins)
		{
			EAssetType asset_type = EAssetType::Skeleton;
			std::string asset_name = as->getAssetName(skin.name, asset_type, asset_indices[asset_type]++, basename);
			URL url = g_runtime_context.fileSystem()->combine(folder, asset_name);

			std::shared_ptr<Skeleton> skeleton = std::make_shared<Skeleton>();
			skeleton->setURL(url);
			skeleton->setName(skin.name);

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
			skeleton->m_bones[skeleton->m_root_bone_index].m_local_bind_pose_transform.fromMatrix(root_bone_matrix);

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
			as->serializeAsset(skeleton);
		}

		// load animations
		for (const tinygltf::Animation& gltf_animation : gltf_model.animations)
		{
			EAssetType asset_type = EAssetType::Animation;
			std::string asset_name = as->getAssetName(gltf_animation.name, asset_type, asset_indices[asset_type]++, basename);
			URL url = g_runtime_context.fileSystem()->combine(folder, asset_name);

			std::shared_ptr<Animation> animation = std::make_shared<Animation>();
			animation->setURL(url);
			animation->setName(gltf_animation.name);

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
						LOG_WARNING("ignore unknown gltf animation sampler's value type {}", accessor.type);
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
				if (gltf_channel.target_path != "rotation" && gltf_channel.target_path != "translation" && gltf_channel.target_path != "scale")
				{
					LOG_WARNING("ignore unknown gltf animation channel's target path {}", gltf_channel.target_path);
					break;
				}

				channel.m_path_type = gltf_channel.target_path == "rotation" ? AnimationChannel::EPathType::Rotation :
					(gltf_channel.target_path == "translation" ? AnimationChannel::EPathType::Translation : AnimationChannel::EPathType::Scale);
				channel.m_bone_name = gltf_model.nodes[gltf_channel.target_node].name;
				channel.m_sampler_index = gltf_channel.sampler;
			}

			animation->inflate();
			as->serializeAsset(animation);
		}

		return true;
	}
}