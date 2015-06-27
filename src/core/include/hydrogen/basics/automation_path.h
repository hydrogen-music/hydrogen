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

	public:
	typedef std::map<float,float>::iterator iterator;
	typedef std::map<float,float>::const_iterator const_iterator;

	private:
	
	float _min;
	float _max;
	float _def;

	std::map<float,float> _points;

	public:
	
	AutomationPath(float min, float max, float def);

	bool empty() const noexcept { return _points.empty(); }
	float get_min() const noexcept { return _min; }
	float get_max() const noexcept { return _max; }
	float get_default() const noexcept { return _def; }

	float get_value(float x) const noexcept;

	void add_point(float x, float y);

	friend bool operator==(const AutomationPath &lhs, const AutomationPath &rhs);
	friend bool operator!=(const AutomationPath &lhs, const AutomationPath &rhs);

	iterator begin() { return _points.begin(); }
	iterator end() { return _points.end(); }
	const_iterator begin() const { return _points.begin(); }
	const_iterator end() const { return _points.end(); }

	iterator find(float x);
	iterator move(iterator &in, float x, float y); 
};

std::ostream &operator<< (std::ostream &o, const AutomationPath &p);

};

#endif
