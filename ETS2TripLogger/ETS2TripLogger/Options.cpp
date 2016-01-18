#include "Options.h"

#include <fstream>
#include <sstream>

Options::Options() :
mOptionsFloat(),
mOptionsInt(),
mOptionsString()
{
}

bool Options::read_file(const std::string& path)
{
	std::ifstream file(path);
	if (!file.is_open())
		return false;

	while (file.good())
	{
		std::string line;
		std::getline(file, line);
		parse_option_line(line);
	}
	return true;
}

float Options::get_option_float(const std::string& key, float def) const
{
	const float_map::const_iterator it = mOptionsFloat.find(key);
	return (it == mOptionsFloat.end()) ? def : it->second;
}

int Options::get_option_int(const std::string& key, int def) const
{
	const int_map::const_iterator it = mOptionsInt.find(key);
	return (it == mOptionsInt.end()) ? def : it->second;
}

const std::string& Options::get_option_string(const std::string& key, const std::string& def) const
{
	const string_map::const_iterator it = mOptionsString.find(key);
	return (it == mOptionsString.end()) ? def : it->second;
}


void Options::parse_option_line(const std::string& line)
{
	std::string key;
	std::string type;
	std::stringstream ss(line);

	ss >> key >> type;
	if (type == "flt")
	{
		float val;
		ss >> val;
		mOptionsFloat[key] = val;
	}
	else if (type == "int")
	{
		int val;
		ss >> val;
		mOptionsInt[key] = val;
	}
	else if (type == "str")
	{
		std::string val;
		ss >> val;
		mOptionsString[key] = val;
	}
}