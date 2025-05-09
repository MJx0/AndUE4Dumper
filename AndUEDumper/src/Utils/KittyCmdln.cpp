#include "KittyCmdln.h"

#include <sstream>
#include <iomanip>

std::string KittyCmdln::getUsage() const
{
    return _usage;
}

void KittyCmdln::setUsage(const std::string &usage)
{
    _usage = usage;
}


cmd_array_t KittyCmdln::getAllCmds() const
{
    return _cmds;
}

cmd_array_t KittyCmdln::getRequirdCmds() const
{
    return _requirdCmds;
}

cmd_array_t KittyCmdln::getOptionalCmds() const
{
    return _optionalCmds;
}

void KittyCmdln::addCmd(const std::string &name, const std::string &alias, const std::string &desc, bool required, const std::function<void()> &callback)
{
    cmd_info_t cmd{};
    cmd.alias = alias;
    cmd.desc = desc;
    cmd.required = required;
    cmd.callback = callback;

    _cmds.push_back({name, cmd});

    if (cmd.required)
        _requirdCmds.push_back({name, cmd});

    if (!cmd.required)
        _optionalCmds.push_back({name, cmd});
}

void KittyCmdln::addFlag(const std::string &name, const std::string &alias, const std::string &desc, bool required, bool *flagPtr)
{
    addCmd(name, alias, desc, required, [flagPtr]()
    {
        *flagPtr = !(*flagPtr);
    });
}


void KittyCmdln::addScanf(const std::string &name, const std::string &alias, const std::string &desc, bool required, const std::string &fmt, void *buffer)
{
    addCmd(name, alias, desc, required, [this, name, fmt, buffer]()
    {
        for (int i = 1; i < _argc; i++)
        {
            if (!strcmp(name.c_str(), _args[i]))
            {
                if ((i + 1) < _argc && !contains(_args[i + 1]))
                {
                    sscanf(_args[i + 1], fmt.c_str(), buffer);
                }
                break;
            }
        }
    });
}


bool KittyCmdln::contains(const std::string &name) const
{
    for (auto &it : _cmds)
    {
        if (it.first == name || it.second.alias == name)
            return true;
    }
    return false;
}

const cmd_info_t *KittyCmdln::find(const std::string &name) const
{
    for (auto &it : _cmds)
    {
        if (it.first == name || it.second.alias == name)
            return &(it.second);
    }
    return nullptr;
}

bool KittyCmdln::requiredCmdsCheck()
{
    for (auto &it : _requirdCmds)
    {
        bool isContained = false;
        for (int i = 1; i < _argc; i++)
        {
            if (it.first == _args[i] || it.second.alias == _args[i])
                isContained = true;
        }
        if (!isContained)
            return false;
    }
    return true;
}

void KittyCmdln::parseArgs()
{
    for (int i = 1; i < _argc; i++)
    {
        auto cmd = find(_args[i]);
        if (cmd != nullptr && cmd->callback != nullptr)
        {
            cmd->callback();
        }
    }
}

std::string KittyCmdln::toString() const
{
    std::stringstream ss;

    ss << _usage << std::endl << std::endl;

    ss << "Required arguments:" << std::endl;
    for (auto &cmd : _requirdCmds)
    {
        std::string cmd_names = cmd.first;
        if (!cmd.second.alias.empty())
            cmd_names += ", " + cmd.second.alias;

        ss << "   " << std::setw(20) << std::left << cmd_names << cmd.second.desc << std::endl;
    }

    ss << std::endl;

    ss << "Optional arguments:" << std::endl;
    for (auto &cmd : _optionalCmds)
    {
        std::string cmd_names = cmd.first;
        if (!cmd.second.alias.empty())
            cmd_names += ", " + cmd.second.alias;

        ss << "   " << std::setw(20) << std::left << cmd_names << cmd.second.desc << std::endl;
    }

    return ss.str();
}
