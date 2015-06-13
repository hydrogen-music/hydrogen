#ifndef H2C_AUTOMATION_PATH_H
#define H2C_AUTOMATION_PATH_H

#include <hydrogen/object.h>

#include <map>

#if __cplusplus <= 199711L
#  define noexcept
#endif

namespace H2Core
{

class AutomationPath : private Object
{
	H2_OBJECT
	
	float _min;
	float _max;
	float _def;

	std::map<float,float> _points;

	public:
	
	AutomationPath(float min, float max, float def);

	float get_min() const noexcept { return _min; }
	float get_max() const noexcept { return _max; }
	float get_default() const noexcept { return _def; }

	float get_value(float x) const noexcept;

	void add_point(float x, float y);
};

};

#endif
