#include "test_helper.h"

#include "hydrogen/object.h"

#include <QProcess>
#include <QProcessEnvironment>
#include <QStringList>
#include <exception>

static const QString APP_DATA_DIR = "/data/";
static const QString TEST_DATA_DIR = "/src/tests/data/";

TestHelper* TestHelper::__instance = NULL;

void TestHelper::create_instance()
{
	if ( __instance == NULL ) {
		__instance = new TestHelper;
	}
}


/**
 * \brief Execute command and return captured output
 * \param args Command to run and its parameters
 * \return Captured standard output
 */
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


/**
 * \brief Check whether direcotry is Hydrogen source root dir
 * \param dir Path to directory
 * \return Whether dir points to Hydrogen source dir
 **/
bool check_root_dir(const QString &dir)
{
	QFile f( dir + TEST_DATA_DIR + "/drumkit/drumkit.xml" );
	return f.exists();
}


/**
 * \brief Try to find Hydrogen source dir
 * \throws std::runtime_error when source dir cannot be find
 *
 * This function tries to find Hydrogen source dir in order to
 * find data files required by tests. First, environment
 * variable H2_HOME is examined. If it's not set or doesn't point
 * to valid directory, this function tries to run "git rev-parse --show-toplevel"
 * to get root directory of git repository. If that fails,
 * current directory is examined. If it doesn't point to
 * source dir, std::runtime_error is thrown.
 **/
QString find_root_dir()
{
	/* Get root dir from H2_HOME env variable */
	auto env_root_dir = QProcessEnvironment::systemEnvironment().value("H2_HOME", "");
	if (env_root_dir != "") {
		if (check_root_dir( env_root_dir ) )
			return env_root_dir;
		else
			___ERRORLOG( QString( "Directory %1 not useable" ).arg( env_root_dir ) );
	}

	/* Try git root directory */
	try {
		auto git_root_dir = qx({"git", "rev-parse", "--show-toplevel"});
		if (check_root_dir( git_root_dir ) )
			return git_root_dir;
		else
			___ERRORLOG( QString( "Directory %1 not useable" ).arg( git_root_dir ) );
	} catch (std::runtime_error &e) {
		___WARNINGLOG( "Can't find git root directory" );
	}

	/* As last resort, use current dir */
	if (check_root_dir( "." ) )
		return ".";
	throw std::runtime_error( "Can't find suitable data directory. Consider setting H2_HOME environment variable" );
}


TestHelper::TestHelper()
{
	auto root_dir = find_root_dir();
	___INFOLOG( QString( "Using test data directrory: %1" ).arg( root_dir ) );
	m_sDataDir = root_dir + APP_DATA_DIR;
	m_sTestDataDir = root_dir + TEST_DATA_DIR;
}


