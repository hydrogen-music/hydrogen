#include <cppunit/TestCase.h>

#include "hydrogen/note.h"


class NoteTest : public CppUnit::TestCase
{
public:
	NoteTest()
			: CppUnit::TestCase( "NoteTest" )
	{}

	void runTest()
	{
		printf("Inizio test per NoteTest\n");

		printf("Test completato\n");
	}
};
