#include "asset_manager.h"
#include "runtime/resource/asset/texture_2d.h"
#include "runtime/resource/asset/material.h"
#include "runtime/resource/asset/static_mesh.h"
#include "runtime/resource/asset/skeletal_mesh.h"
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <fstream>
#include <queue>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>
#include <ktx/ktx.h>

#define ARCHIVE_ASSET(type, asset) \
	case EAssetType:: ##type: \
		archive(std::dynamic_pointer_cast<##type>(asset)); \
	break

#define REFERENCE_ASSET(object, prop_name, ref_asset) \
	object-> ##prop_name = ref_asset; \
	object->m_ref_urls[#prop_name] = ref_asset->getURL()

namespace Bamboo
{
	void AssetManager::init()
	{
		m_asset_exts = {
			{ EAssetType::Texture2D, "tex"},
			{ EAssetType::TextureCube, "texc"}, 
			{ EAssetType::Material, "mat"}, 
			{ EAssetType::Skeleton, "skl"},
			{ EAssetType::StaticMesh, "sm"}, 
			{ EAssetType::SkeletalMesh, "skm"}, 
			{ EAssetType::Animation, "anim"}
		};
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

	std::shared_ptr<Asset> AssetManager::loadAssetImpl(const URL& url)
	{
		return nullptr;
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
			std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>(url);
			texture->loadFromGltf(gltf_image, gltf_sampler);

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

			std::shared_ptr<Material> material = std::make_shared<Material>(url);

			material->m_base_color_factor = glm::make_vec4(gltf_material.pbrMetallicRoughness.baseColorFactor.data());
			material->m_emissive_factor = glm::make_vec4(gltf_material.emissiveFactor.data());
			material->m_metallic_factor = gltf_material.pbrMetallicRoughness.metallicFactor;
			material->m_roughness_factor = gltf_material.pbrMetallicRoughness.roughnessFactor;

			if (gltf_material.pbrMetallicRoughness.baseColorTexture.index != INVALID_INDEX)
			{
				REFERENCE_ASSET(material, m_base_color_texure, textures[gltf_material.pbrMetallicRoughness.baseColorTexture.index]);
				material->m_base_color_texure->setTextureType(TextureType::BaseColor);
				ASSERT(gltf_material.pbrMetallicRoughness.baseColorTexture.texCoord == 0, "do not support non-zero texture coordinate index");
			}
			if (gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.index != INVALID_INDEX)
			{
				REFERENCE_ASSET(material, m_metallic_roughness_texure, textures[gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.index]);
				material->m_metallic_roughness_texure->setTextureType(TextureType::MetallicRoughness);
				ASSERT(gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord == 0, "do not support non-zero texture coordinate index");
			}
			if (gltf_material.normalTexture.index != INVALID_INDEX)
			{
				REFERENCE_ASSET(material, m_normal_texure, textures[gltf_material.normalTexture.index]);
				material->m_normal_texure->setTextureType(TextureType::Normal);
				ASSERT(gltf_material.normalTexture.texCoord == 0, "do not support non-zero texture coordinate index");
			}
			if (gltf_material.occlusionTexture.index != INVALID_INDEX)
			{
				REFERENCE_ASSET(material, m_occlusion_texure, textures[gltf_material.occlusionTexture.index]);
				material->m_occlusion_texure->setTextureType(TextureType::Occlusion);
				ASSERT(gltf_material.occlusionTexture.texCoord == 0, "do not support non-zero texture coordinate index");
			}
			if (gltf_material.emissiveTexture.index != INVALID_INDEX)
			{
				REFERENCE_ASSET(material, m_emissive_texure, textures[gltf_material.emissiveTexture.index]);
				material->m_emissive_texure->setTextureType(TextureType::Emissive);
				ASSERT(gltf_material.emissiveTexture.texCoord == 0, "do not support non-zero texture coordinate index");
			}

			serializeAsset(material);
			materials.push_back(material);
			m_assets[url] = material;
		}

		// 3.load nodes recursively
		for (const tinygltf::Scene& gltf_scene : gltf_model.scenes)
		{
			std::queue<const tinygltf::Node*> nodes;
			for (int index : gltf_scene.nodes)
			{
				nodes.push(&gltf_model.nodes[index]);
			}

			while (!nodes.empty())
			{
				const tinygltf::Node* node = nodes.front();
				nodes.pop();

				if (node->mesh == INVALID_INDEX)
				{
					LOG_INFO("ignore non-mesh gltf node");
					continue;
				}

				glm::mat4 local_matrix = glm::mat4(1.0f);
				if (!node->matrix.empty())
				{
					local_matrix = glm::make_mat4x4(node->matrix.data());
				}

				const tinygltf::Mesh gltf_mesh = gltf_model.meshes[node->mesh];
				ASSERT(!gltf_mesh.primitives.empty(), "gltf mesh's primitives shouldn't be empty");

				// check whether it's a skeletal mesh 
				const tinygltf::Primitive& first_primitive = gltf_mesh.primitives.front();
				bool is_static_mesh = first_primitive.attributes.find("JOINTS_0") == first_primitive.attributes.end() ||
					first_primitive.attributes.find("WEIGHTS_0") == first_primitive.attributes.end();

				EAssetType asset_type = is_static_mesh ? EAssetType::StaticMesh : EAssetType::SkeletalMesh;
				std::string asset_name = getAssetName(basename, gltf_mesh.name, asset_type, asset_indices[asset_type]++);
				URL url = g_runtime_context.fileSystem()->combine(folder, asset_name);
				size_t vertex_count = 0, index_count = 0;
				size_t primitive_count = gltf_mesh.primitives.size();
				for (const tinygltf::Primitive& primitive : gltf_mesh.primitives)
				{
					ASSERT(primitive.attributes.find("POSITION") != primitive.attributes.end(), "gltf primitives must have position");
					ASSERT(primitive.attributes.find("NORMAL") != primitive.attributes.end(), "gltf primitives must have normal");
					ASSERT(primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end(), "gltf primitives must have 0th texture coordinate");
					ASSERT(primitive.indices != INVALID_INDEX, "gltf primitive must have indices");

					vertex_count += gltf_model.accessors[primitive.attributes.find("POSITION")->second].count;
					index_count += gltf_model.accessors[primitive.indices].count;
				}

				std::shared_ptr<StaticMesh> static_mesh = nullptr;
				std::shared_ptr<SkeletalMesh> skeletal_mesh = nullptr;
				std::shared_ptr<Mesh> mesh = nullptr;
				if (is_static_mesh)
				{
					static_mesh = std::make_shared<StaticMesh>(url);
					static_mesh->m_sub_meshes.resize(primitive_count);
					static_mesh->m_vertices.resize(vertex_count);
					static_mesh->m_indices.resize(index_count);
					mesh = static_mesh;
				}
				else
				{
					skeletal_mesh = std::make_shared<SkeletalMesh>(url);
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
					const tinygltf::Primitive& primitive = gltf_mesh.primitives[p];

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
					if (!is_static_mesh)
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
						if (is_static_mesh)
						{
							static_vertex = &static_mesh->m_vertices[v + vertex_start];
						}
						else
						{
							skeletal_vertex = &skeletal_mesh->m_vertices[v + vertex_start];
							static_vertex = skeletal_vertex;
						}

						static_vertex->position = glm::make_vec3(&position_buffer[v * position_byte_stride]);
						static_vertex->tex_coord = glm::make_vec2(&tex_coord_buffer[v * tex_coord_byte_stride]);
						static_vertex->normal = glm::make_vec3(&normal_buffer[v * normal_byte_stride]);

						if (!is_static_mesh)
						{
							switch (joint_component_type)
							{
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: 
							{
								const uint16_t* joint_buffer = static_cast<const uint16_t*>(joint_void_buffer);
								skeletal_vertex->bones = glm::vec4(glm::make_vec4(&joint_buffer[v * joint_byte_stride]));
								break;
							}
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: 
							{
								const uint8_t* joint_buffer = static_cast<const uint8_t*>(joint_void_buffer);
								skeletal_vertex->bones = glm::vec4(glm::make_vec4(&joint_buffer[v * joint_byte_stride]));
								break;
							}
							default:
								LOG_FATAL("unknow gltf mesh joint component type");
								break;
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
							static_mesh->m_indices[index_idx++] = index_buffer[i] + vertex_start;
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: 
					{
						const uint16_t* index_buffer = static_cast<const uint16_t*>(index_void_buffer);
						for (size_t i = 0; i < primitive_index_count; ++i)
						{
							static_mesh->m_indices[index_idx++] = index_buffer[i] + vertex_start;
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: 
					{
						const uint8_t* index_buffer = static_cast<const uint8_t*>(index_void_buffer);
						for (size_t i = 0; i < primitive_index_count; ++i)
						{
							static_mesh->m_indices[index_idx++] = index_buffer[i] + vertex_start;
						}
						break;
					}
					default:
						LOG_FATAL("unknow gltf mesh index component type");
						break;
					}

					// set submesh
					SubMesh* sub_mesh = &mesh->m_sub_meshes[p];
					sub_mesh->m_first_index = index_start;
					sub_mesh->m_index_count = primitive_index_count;
					sub_mesh->m_vertex_count = primitive_vertex_count;
					sub_mesh->m_bounding_box = BoundingBox{ min_position, max_position };
					REFERENCE_ASSET(sub_mesh, m_material, materials[primitive.material]);

					vertex_start += primitive_vertex_count;
					index_start += primitive_index_count;
				}
			}
		}

		return true;
	}

	std::string AssetManager::getAssetName(const std::string& basename, const std::string& asset_name, EAssetType asset_type, int asset_index)
	{
		const std::string& ext = m_asset_exts[asset_type];
		if (!asset_name.empty())
		{
			std::string asset_basename = g_runtime_context.fileSystem()->basename(asset_name);
			return g_runtime_context.fileSystem()->format("%s_%s.%s", ext.c_str(), asset_basename.c_str(), ext.c_str());
		}
		return g_runtime_context.fileSystem()->format("%s_%s_%d.%s", ext.c_str(), basename.c_str(), asset_index, ext.c_str());
	}

	void AssetManager::serializeAsset(std::shared_ptr<Asset> asset)
	{
		std::string filename = TO_ABSOLUTE(asset->getURL());
		std::ofstream ofs(filename);

		switch (asset->getArchiveType())
		{
		case EArchiveType::Json:
		{
			cereal::JSONOutputArchive archive(ofs);

			switch (asset->getAssetType())
			{
				ARCHIVE_ASSET(Texture2D, asset);
				ARCHIVE_ASSET(Material, asset);
			default:
				break;
			}
		}
		break;
		case EArchiveType::Binary:
		{
			cereal::BinaryOutputArchive archive(ofs);
			
			switch (asset->getAssetType())
			{
				ARCHIVE_ASSET(Texture2D, asset);
				ARCHIVE_ASSET(Material, asset);
			default:
				break;
			}
		}
		break;
		default:
			break;
		}
	}

	void AssetManager::deserializeAsset(std::shared_ptr<Asset> asset)
	{

	}
}