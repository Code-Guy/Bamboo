#include "file_watcher.h"
#include "engine/core/base/macro.h"
#include "engine/resource/asset/asset_manager.h"

namespace Bamboo
{

	void FolderNode::setDir(const std::string& dir)
	{
		this->dir = dir;
		name = g_engine.fileSystem()->basename(dir);
	}

	void FileWatcher::init()
	{
		// initialize file watcher
		const auto& fs = g_engine.fileSystem();
		std::string asset_dir = fs->getAssetDir();
		m_file_watcher = std::make_unique<filewatch::FileWatch<std::string>>(
			asset_dir,
			[this, asset_dir](const std::string& path, const filewatch::Event event)
			{
				std::string ppath = std::filesystem::path(asset_dir).append(path).generic_string();
				bool is_dir = !std::filesystem::path(ppath).has_extension();
				if (!is_dir && !isValidAssetFile(ppath))
				{
					return;
				}

				static std::string rename_file;
				if (event == filewatch::Event::renamed_old)
				{
					rename_file = ppath;
					return;
				}

				FileChange file_change;
				file_change.path = ppath;
				file_change.type = (int)event > (int)(filewatch::Event::modified) ? EFileChangeType::Renamed : (EFileChangeType)event;
				file_change.is_dir = is_dir;
				if (event == filewatch::Event::renamed_new)
				{
					file_change.old_path = rename_file;
				}
				m_file_change_queue.push(file_change);
			}
		);

		// initialize root folder node
		m_root_folder_node = std::make_shared<FolderNode>();
		m_root_folder_node->setDir(asset_dir);
		m_root_folder_node->is_root = true;

		// recursively traverse the root folder by a queue
		std::queue<std::shared_ptr<FolderNode>> folder_queue;
		folder_queue.push(m_root_folder_node);
		while (!folder_queue.empty())
		{
			std::shared_ptr<FolderNode> folder_node = folder_queue.front();
			folder_queue.pop();

			m_folder_node_map[folder_node->dir] = folder_node;
			for (auto& file : std::filesystem::directory_iterator(folder_node->dir))
			{
				std::string filename = file.path().generic_string();

				if (isValidAssetFile(filename))
				{
					folder_node->child_files.insert(filename);
				}
				else if (file.is_directory())
				{
					std::shared_ptr<FolderNode> child_folder_node = std::make_shared<FolderNode>();
					child_folder_node->setDir(filename);
					child_folder_node->parent_folder = folder_node;

					folder_node->child_folders.insert(child_folder_node);
					folder_queue.push(child_folder_node);
				}
			}
			folder_node->is_leaf = folder_node->child_folders.empty();
		}
	}

	void FileWatcher::tick()
	{
		static std::map<EFileChangeType, std::string> file_change_type_name_map = {
			{ EFileChangeType::Added, "added" },
			{ EFileChangeType::Removed, "removed" },
			{ EFileChangeType::Modified, "modified" },
			{ EFileChangeType::Renamed, "rename" }
		};

		while (!m_file_change_queue.empty())
		{
			FileChange file_change = m_file_change_queue.pop();
			EFileChangeType type = file_change.type;
			const std::string& path = file_change.path;
			const std::string& old_path = file_change.old_path;
			if (file_change.is_dir)
			{
				switch (type)
				{
				case EFileChangeType::Added:
				{
					std::shared_ptr<FolderNode> folder_node = std::make_shared<FolderNode>();
					folder_node->setDir(path);
					folder_node->is_leaf = true;

					std::string parent_path = g_engine.fileSystem()->dir(path);
					std::shared_ptr<FolderNode>& parent_folder = m_folder_node_map[parent_path];
					folder_node->parent_folder = parent_folder;
					parent_folder->child_folders.insert(folder_node);

					m_folder_node_map[path] = folder_node;
				}
					break;
				case EFileChangeType::Removed:
				{
					std::shared_ptr<FolderNode>& folder_node = m_folder_node_map[path];
					removeFolderNode(folder_node);
				}
					break;
				case EFileChangeType::Modified:
				{
					// just ignore folder modification
				}
					break;
				case EFileChangeType::Renamed:
				{
					std::shared_ptr<FolderNode>& folder_node = m_folder_node_map[old_path];
					folder_node->setDir(path);
				}
					break;
				default:
					break;
				}
			}
			else
			{
				// find which folder contains this file
				std::string parent_path = g_engine.fileSystem()->dir(path);
				std::shared_ptr<FolderNode>& folder_node = m_folder_node_map[parent_path];

				switch (type)
				{
				case EFileChangeType::Added:
				{
					folder_node->child_files.insert(path);
				}
				break;
				case EFileChangeType::Removed:
				{
					folder_node->child_files.erase(path);
				}
				break;
				case EFileChangeType::Modified:
				{
					// TODO when file modified, reload asset
				}
				break;
				case EFileChangeType::Renamed:
				{
					folder_node->child_files.erase(old_path);
					folder_node->child_files.insert(path);
				}
				break;
				default:
					break;
				}
			}

			if (!(file_change.is_dir && type == EFileChangeType::Modified))
			{
				LOG_INFO("path {} {}{}", path, file_change_type_name_map[type],
					type != EFileChangeType::Renamed ? "" : (" , old_path: " + old_path));
			}
		}
	}

	void FileWatcher::destroy()
	{
		m_file_watcher.reset();
		removeFolderNode(m_root_folder_node);
		m_root_folder_node.reset();
	}

	void FileWatcher::getFolderFiles(const std::string& folder, std::vector<std::string>& folder_files)
	{
		folder_files.clear();
		if (m_folder_node_map.find(folder) != m_folder_node_map.end())
		{
			std::shared_ptr<FolderNode>& folder_node = m_folder_node_map[folder];

			// add child folders
			for (const auto& child_folder : folder_node->child_folders)
			{
				folder_files.push_back(child_folder->dir);
			}

			// add child files
			for (const auto& child_file : folder_node->child_files)
			{
				folder_files.push_back(child_file);
			}
		}
	}

	void FileWatcher::getAllFiles(std::vector<std::string>& all_files)
	{
		all_files.clear();
		for (const auto& iter : m_folder_node_map)
		{
			for (const auto& child_file : iter.second->child_files)
			{
				all_files.push_back(child_file);
			}
		}
	}

	void FileWatcher::removeFolderNode(std::shared_ptr<FolderNode> folder_node)
	{
		m_folder_node_map.erase(folder_node->dir);
		if (folder_node->parent_folder.lock())
		{
			folder_node->parent_folder.lock()->child_folders.erase(folder_node);
		}
		
		auto child_folders = folder_node->child_folders;
		for (auto child_folder_node : child_folders)
		{
			removeFolderNode(child_folder_node);
		}
		folder_node.reset();
	}

	bool FileWatcher::isValidAssetFile(const std::string& filename)
	{
		return std::filesystem::is_regular_file(filename) && g_engine.assetManager()->getAssetType(filename) != EAssetType::Invalid;
	}

}