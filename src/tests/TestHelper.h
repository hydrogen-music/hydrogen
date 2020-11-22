#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <QString>
#include <cassert>

class TestHelper {
	static TestHelper*	m_pInstance;
	QString m_sDataDir;
	QString m_sTestDataDir;
	
	public:
		TestHelper();
	
		QString getDataDir() const;
		QString getTestFile(const QString& file);
	
		static void			createInstance();
		static TestHelper*	get_instance();
};

inline TestHelper*	TestHelper::get_instance() 
{ 
	assert(m_pInstance); return m_pInstance; 
}

inline QString TestHelper::getDataDir() const 
{ 
	return m_sDataDir; 
}

inline QString TestHelper::getTestFile(const QString& file)
{
	return m_sTestDataDir + "/" + file; 
}


#define H2TEST_FILE(name) TestHelper::get_instance()->getTestFile(name)

#endif
