#pragma once

#include <map>
#include <set>
#include <file_watch/file_watch.hpp>
#include "engine/platform/container/ts_queue.h"

namespace Bamboo
{
	enum class EFileChangeType
	{
		Added, Removed, Modified, Renamed
	};

	struct FileChange
	{
		std::string path;
		EFileChangeType type;
		bool is_dir = false;

		// if type is Renamed, we need old_filename
		std::string old_path;
	};

	struct FolderNode
	{
		class FolderNodePtrCompare
		{
		public:
			bool operator()(const std::shared_ptr<FolderNode>& lhs,
				const std::shared_ptr<FolderNode>& rhs) const
			{
				return lhs->name < rhs->name;
			}
		};

		std::string name;
		std::string dir;

		std::set<std::string> child_files;
		std::set<std::shared_ptr<FolderNode>, FolderNodePtrCompare> child_folders;
		std::weak_ptr<FolderNode> parent_folder;

		bool is_root = false;
		bool is_leaf = false;

		void setDir(const std::string& dir);
	};

	class FileWatcher
	{
	public:
		void init();
		void tick();
		void destroy();

		static FileWatcher& get()
		{
			static FileWatcher instance;
			return instance;
		}

		std::shared_ptr<FolderNode>& getRootFolderNode() { return m_root_folder_node; }
		void getFolderFiles(const std::string& folder, std::vector<std::string>& folder_files);
		void getAllFiles(std::vector<std::string>& all_files);

	private:
		FileWatcher() = default;
		~FileWatcher() = default;

		void removeFolderNode(std::shared_ptr<FolderNode> folder_node);
		bool isValidAssetFile(const std::string& filename);

		std::unique_ptr<filewatch::FileWatch<std::string>> m_file_watcher;
		TSQueue<FileChange> m_file_change_queue;

		std::shared_ptr<FolderNode> m_root_folder_node = nullptr;
		std::map<std::string, std::shared_ptr<FolderNode>> m_folder_node_map;
	};
}