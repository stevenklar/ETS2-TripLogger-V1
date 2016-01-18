#ifndef OPTIONS_H
#define OPTIONS_H

#include <map>
#include <string>

class Options
{
public:
	Options();

	bool read_file(const std::string& path);

	float get_option_float(const std::string& key, float def) const;
	int get_option_int(const std::string& key, int def) const;
	const std::string& get_option_string(const std::string& key, const std::string& def) const;

private:
	void parse_option_line(const std::string& line);

	typedef std::map<std::string, float> float_map;
	typedef std::map<std::string, int> int_map;
	typedef std::map<std::string, std::string> string_map;

	float_map   mOptionsFloat;
	int_map     mOptionsInt;
	string_map  mOptionsString;

};

#endif