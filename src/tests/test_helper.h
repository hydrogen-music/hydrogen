#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <QString>
#include <cassert>

class TestHelper {
	static TestHelper *	__instance;
	QString m_sDataDir;
	QString m_sTestDataDir;
	public:
	TestHelper();

	QString data_dir() const { return m_sDataDir; }
	QString test_file(const QString &file) { return m_sTestDataDir + "/" + file; }

	static void			create_instance();
	static TestHelper* get_instance() { assert(__instance); return __instance; }
};

#define H2TEST_FILE(name) TestHelper::get_instance()->test_file(name)

#endif
