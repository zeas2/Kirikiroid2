#pragma once
#include <string>
#include <vector>
#include <functional>

class XP3ArchiveRepackAsyncImpl;
class XP3ArchiveRepackAsync {

public:
	XP3ArchiveRepackAsync();
	~XP3ArchiveRepackAsync();
	uint64_t AddTask(const std::string &src);
	void Start();
	void Stop();
	void SetXP3Filter(const std::string &xp3filter);
	void SetCallback(
		const std::function<void(int, uint64_t, const std::string &)> &onNewFile,
		const std::function<void(int, uint64_t, const std::string &)> &onNewArchive,
		const std::function<void(uint64_t, uint64_t, uint64_t)> &onProgress,
		const std::function<void(int, const std::string &)> &onError,
		const std::function<void()> &onEnded);

	void SetOption(const std::string &name, bool v);

private:
	XP3ArchiveRepackAsyncImpl *_impl;
};
