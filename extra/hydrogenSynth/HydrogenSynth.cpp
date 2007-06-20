#include <iostream>
using namespace std;

#include <hydrogen/audio_engine.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/fx/Effects.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/note.h>

#include <QApplication>

int main( int argc, char* argv[] )
{
	cout << "--=( Hydrogen Synth test )=--" << endl;

	QApplication a(argc, argv);

	Object::useVerboseLog( true );

	H2Core::Hydrogen *pHydrogen = H2Core::Hydrogen::getInstance();

	H2Core::AudioEngine *pEngine = H2Core::AudioEngine::getInstance();

	H2Core::Synth *pSynth = pEngine->getSynth();



	char pippo;

	while (true) {
		pippo = getchar();
		switch( pippo ) {
			case 'a':
				{
				cout << "note on" << endl;
				H2Core::NoteKey key;
				key.m_key = H2Core::NoteKey::C;
				H2Core::Note *pNote = new H2Core::Note( 0, 0, 0.8, 1.0, 1.0, -1, 0.0, key );

				pSynth->noteOn( pNote );

				}
				break;

			case 's':
				{
				cout << "note off" << endl;
				H2Core::NoteKey key;
				key.m_key = H2Core::NoteKey::C;
				H2Core::Note *pNote = new H2Core::Note( 0, 0, 0.8, 1.0, 1.0, -1, 0.0, key );
				pSynth->noteOff( pNote );
				}
				break;

			case 'q':
				cout << endl << "shutdown..." << endl;
				pHydrogen->sequencer_stop();

				delete H2Core::AudioEngine::getInstance();
				delete H2Core::EventQueue::getInstance();
				delete H2Core::Preferences::getInstance();
				delete Logger::getInstance();

				std::cout << std::endl << std::endl << Object::getNObjects() << " alive objects" << std::endl << std::endl;
				Object::printObjectMap();

				return 0;
				break;
		}
	}

	cout << "bye..." << endl;
	return 0;
}

