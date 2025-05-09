#pragma once

#include <functional>
#include <vector>
#include <utility>
#include <string>

struct cmd_info_t
{
	std::string alias, desc;
	bool required;
	std::function<void()> callback;
};

using cmd_array_t = std::vector<std::pair<std::string, cmd_info_t>>;

class KittyCmdln
{
private:
	std::string _usage;

	cmd_array_t _cmds;
	cmd_array_t _requirdCmds;
	cmd_array_t _optionalCmds;

	int _argc;
	char** _args;

public:
	KittyCmdln(int argc, char** args) : _argc(argc), _args(args) {}

	std::string getUsage() const;
	void setUsage(const std::string& usage);

	cmd_array_t getAllCmds() const;
	cmd_array_t getRequirdCmds() const;
	cmd_array_t getOptionalCmds() const;

	void addCmd(const std::string& name, const std::string& alias, const std::string& desc, bool required, const std::function<void()>& callback);
	void addFlag(const std::string& name, const std::string& alias, const std::string& desc, bool required, bool* flagPtr);
	void addScanf(const std::string& name, const std::string& alias, const std::string& desc, bool required, const std::string& fmt, void* buffer);

	bool contains(const std::string& name) const;

	const cmd_info_t* find(const std::string& name) const;

	bool requiredCmdsCheck();

	void parseArgs();

	std::string toString() const;
};