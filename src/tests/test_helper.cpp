#include "test_helper.h"

#include <QProcess>
#include <QStringList>
#include <exception>

TestHelper* TestHelper::__instance = NULL;

void TestHelper::create_instance()
{
	if ( __instance == NULL ) {
		__instance = new TestHelper;
	}
}

QString qx(QStringList args)
{
	QProcess proc;
	proc.start(args.first(), args.mid(1));
	proc.waitForFinished();
	if ( proc.exitCode() != 0 )
		throw std::runtime_error("Not a git repository");
	auto path = proc.readAllStandardOutput();
	return QString(path).trimmed();
}


TestHelper::TestHelper()
{
	auto git_dir = qx({"git", "rev-parse", "--show-toplevel"});
	m_sDataDir = git_dir + "/data";
	m_sTestDataDir = git_dir + "/src/tests/data";
}


