#pragma once

struct GltfImportOption
{
	// mesh
	bool combine_meshes;
	bool force_static_mesh;

	// material
	bool contains_occlusion_channel;
};