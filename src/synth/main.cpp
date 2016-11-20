#include <iostream>
using namespace std;

#include <hydrogen/audio_engine.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/fx/Effects.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/note.h>

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

int main( int argc, char* argv[] )
{
	cout << "--=( Hydrogen Synth test )=--" << endl;

	QApplication a(argc, argv);

    int log_level = H2Core::Logger::Debug | H2Core::Logger::Info | H2Core::Logger::Warning | H2Core::Logger::Error;
    H2Core::Logger* logger = H2Core::Logger::bootstrap( log_level );
    H2Core::Object::bootstrap( logger, logger->should_log(H2Core::Logger::Debug) );

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
				delete H2Core::Logger::get_instance();

				std::cout << std::endl << std::endl << H2Core::Object::objects_count() << " alive objects" << std::endl << std::endl;
                H2Core::Object::write_objects_map_to_cerr();

				return 0;
				break;
		}
	}

	cout << "bye..." << endl;
	return 0;
}

