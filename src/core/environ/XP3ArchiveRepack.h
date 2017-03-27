#pragma once
#include <string>
#include <vector>

class tTVPXP3ArchiveEx;
class XP3ArchiveRepackAsync {
	std::vector<std::pair<tTVPXP3ArchiveEx*, std::string> > ConvFileList;
	void * /*std::thread*/ Thread = nullptr;

public:
	XP3ArchiveRepackAsync(const std::string &xp3filter);
	~XP3ArchiveRepackAsync();
	uint64_t AddTask(const std::string &src/*, const std::string &dst*/);
	void Clear();
	void Start();
	void DoConv();
	
public:
	bool UsingETC2 = false;
};
