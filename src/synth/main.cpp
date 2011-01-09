#include <iostream>
using namespace std;

#include <hydrogen/audio_engine.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/fx/Effects.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/note.h>

#include <QtGui>

int main( int argc, char* argv[] )
{
	cout << "--=( Hydrogen Synth test )=--" << endl;

	QApplication a(argc, argv);

    int log_level = Logger::Debug | Logger::Info | Logger::Warning | Logger::Error;
    Logger* logger = Logger::bootstrap( log_level );
    Object::bootstrap( logger, logger->should_log(Logger::Debug) );

	H2Core::Hydrogen *pHydrogen = H2Core::Hydrogen::get_instance();

	H2Core::AudioEngine *pEngine = H2Core::AudioEngine::get_instance();

	H2Core::Synth *pSynth = pEngine->get_synth();



	char pippo;

	while (true) {
		pippo = getchar();
		switch( pippo ) {
			case 'a':
				{
				cout << "note on" << endl;
				H2Core::Note *pNote = new H2Core::Note( 0, 0, 0.8, 1.0, 1.0, -1, 0.0 );

				pSynth->noteOn( pNote );

				}
				break;

			case 's':
				{
				cout << "note off" << endl;
				H2Core::Note *pNote = new H2Core::Note( 0, 0, 0.8, 1.0, 1.0, -1, 0.0 );
				pSynth->noteOff( pNote );
				}
				break;

			case 'q':
				cout << endl << "shutdown..." << endl;
				pHydrogen->sequencer_stop();

				delete H2Core::AudioEngine::get_instance();
				delete H2Core::EventQueue::get_instance();
				delete H2Core::Preferences::get_instance();
				delete Logger::get_instance();

				std::cout << std::endl << std::endl << Object::objects_count() << " alive objects" << std::endl << std::endl;
				Object::write_objects_map_to_cerr();

				return 0;
				break;
		}
	}

	cout << "bye..." << endl;
	return 0;
}

