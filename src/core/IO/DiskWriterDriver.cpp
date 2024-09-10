/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */
#include <unistd.h>


#include <core/AudioEngine/AudioEngine.h>
#include <core/EventQueue.h>
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Sample.h>
#include <core/IO/DiskWriterDriver.h>

#include <pthread.h>
#include <cassert>

#if defined(WIN32) || _DOXYGEN_
#include <windows.h>
/*
 * In Windows the unistd function sleep( seconds ) is not available.
 * Treat sleep( SECONDS ) as a macro that uses SleepEx.
 * Convert seconds to milliseconds for the first argument of SleepEx.
 * Use false for the second argument of SleepEx.
 * This way SleepEx always returns 0, after the specified time has passed.
 */
#define sleep( SECONDS ) SleepEx( SECONDS * 1000, false )
#endif

namespace H2Core
{

pthread_t diskWriterDriverThread;

void* diskWriterDriver_thread( void* param )
{
	Base * __object = ( Base * )param;
	DiskWriterDriver *pDriver = ( DiskWriterDriver* )param;

	EventQueue::get_instance()->push_event( EVENT_PROGRESS, 0 );

	auto pAudioEngine = Hydrogen::get_instance()->getAudioEngine();
	
	__INFOLOG( QString( "DiskWriterDriver thread started using libsndfile version [%1]" )
			   .arg( QString( sf_version_string() ) ) );


	// always rolling, no user interaction
	pAudioEngine->play();
	SF_INFO soundInfo;
	soundInfo.samplerate = pDriver->m_nSampleRate;
//	soundInfo.frames = -1;//getNFrames();		///\todo: da terminare
	soundInfo.channels = 2;
	//default format
	int sfformat = 0x010000; //wav format (default)
	int bits = 0x0002; //16 bit PCM (default)
	//sf_format switch

	// Determine audio format based on the provided file suffix.
	const QString sFilenameLower = pDriver->m_sFilename.toLower();
	if ( sFilenameLower.endsWith( ".aiff" ) ||
		 sFilenameLower.endsWith( ".aif" ) ||
 		 sFilenameLower.endsWith( ".aifc" ) ) {
		sfformat =  0x020000; //Apple/SGI AIFF format (big endian)
	}
	else if ( sFilenameLower.endsWith( ".flac" ) ) {
		sfformat =  0x170000; //FLAC lossless file format
	}
	else if ( sFilenameLower.endsWith( ".wav" ) ) {
		sfformat =  0x010000;
	}
	else if ( sFilenameLower.endsWith( ".au" ) ) {
		sfformat =  0x030000;
	}
	else if ( sFilenameLower.endsWith( ".caf" ) ) {
		sfformat =  0x180000;
	}
	else if ( sFilenameLower.endsWith( ".w64" ) ) {
		sfformat =  0x0B0000;
	}
	else if ( sFilenameLower.endsWith( ".ogg" ) ) {
		sfformat = SF_FORMAT_OGG | SF_FORMAT_VORBIS;
	}
	else if ( sFilenameLower.endsWith( ".voc" ) ) {
		sfformat =  0x080000;
	}
	else if ( sFilenameLower.endsWith( ".mp3" ) ) {
		sfformat =  0x230000;
	}
	else {
		__ERRORLOG( QString( "Unsupported file extension [%1]" )
					.arg( pDriver->m_sFilename ) );
		pDriver->m_bDoneWriting = true;
		pDriver->m_bWritingFailed = true;
		pthread_exit( nullptr );
		return nullptr;

	}

	// Handle sample depth
	if ( pDriver->m_nSampleDepth == 8 ) {
		// WAV and raw PCM data are handled differently.
		if ( sFilenameLower.endsWith(".wav") ) {
			bits = 0x0005; //Unsigned 8 bit data needed for Microsoft WAV format
		} else {
			bits = 0x0001; //Signed 8 bit data works with aiff
		}
	}
	else if ( pDriver->m_nSampleDepth == 16 ) {
		bits = 0x0002; //Signed 16 bit data
	}
	else if ( pDriver->m_nSampleDepth == 24 ) {
		bits = 0x0003; //Signed 24 bit data
	}
	else if ( pDriver->m_nSampleDepth == 32 ) {
		bits = 0x0004; ////Signed 32 bit data
	}

	if ( sFilenameLower.endsWith( ".ogg" ) ) {
		soundInfo.format =  sfformat;
	} else {
		soundInfo.format =  sfformat|bits;
	}

	if ( !sf_format_check( &soundInfo ) ) {
		__ERRORLOG( "Error in soundInfo" );
		pDriver->m_bDoneWriting = true;
		pDriver->m_bWritingFailed = true;
		pthread_exit( nullptr );
		return nullptr;
	}

#ifdef WIN32
	// On Windows we use a special version of sf_open to ensure we get all
	// characters of the filename entered in the GUI right. No matter which
	// encoding was used locally.
	// We have to terminate the string using a null character ourselves.
	QString sPaddedPath = pDriver->m_sFilename.append( '\0' );
	wchar_t* encodedFilename = new wchar_t[ sPaddedPath.size() ];

	sPaddedPath.toWCharArray( encodedFilename );
	
	SNDFILE* m_file = sf_wchar_open( encodedFilename, SFM_WRITE,
								   &soundInfo );
	delete encodedFilename;
#else
	SNDFILE* m_file = sf_open( pDriver->m_sFilename.toLocal8Bit(), SFM_WRITE,
							   &soundInfo );
#endif

	if ( m_file == nullptr ) {
		__ERRORLOG( QString( "Unable to open file [%1] with format [%2]: %3" )
					.arg( pDriver->m_sFilename )
					.arg( Sample::sndfileFormatToQString( soundInfo.format ) )
					.arg( Sample::sndfileErrorToQString( sf_error( nullptr ) ) ) );
		pDriver->m_bDoneWriting = true;
		pDriver->m_bWritingFailed = true;
		pthread_exit( nullptr );
		return nullptr;
	}

	float *pData = new float[ pDriver->m_nBufferSize * 2 ]; // always stereo

	float *pData_L = pDriver->m_pOut_L;
	float *pData_R = pDriver->m_pOut_R;

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pSampler = pHydrogen->getAudioEngine()->getSampler();

	std::vector<PatternList*> *pPatternColumns = pSong->getPatternGroupVector();
	int nColumns = pPatternColumns->size();

	// Used to cleanly terminate this thread and close all handlers.
	auto tearDown = [&](){
		pDriver->m_bDoneWriting = true;
		delete[] pData;
		pData = nullptr;

		sf_close( m_file );

		__INFOLOG( "DiskWriterDriver thread end" );

		pthread_exit( nullptr );
	};
	
	int nPatternSize, nBufferWriteLength;
	float fBpm;
	float fTicksize = 0;
	int nMaxNumberOfSilentFrames = 200;
	for ( int patternPosition = 0; patternPosition < nColumns; ++patternPosition ) {
		
		PatternList *pColumn = ( *pPatternColumns )[ patternPosition ];
		if ( pColumn->size() != 0 ) {
			nPatternSize = pColumn->longest_pattern_length();
		} else {
			nPatternSize = MAX_NOTES;
		}

		fBpm = AudioEngine::getBpmAtColumn( patternPosition );
		fTicksize = AudioEngine::computeTickSize( pDriver->m_nSampleRate, fBpm,
												  pSong->getResolution() );
		
		//here we have the pattern length in frames dependent from bpm and samplerate
		int nPatternLengthInFrames = fTicksize * nPatternSize;
		int nFrameNumber = 0;
		int nLastRun = 0;
		int nSuccessiveZeros = 0;
		while ( ( patternPosition < nColumns - 1 && // render all
													// frames in
													// pattern 
				  nFrameNumber < nPatternLengthInFrames ) ||
				( patternPosition == nColumns - 1 && // render till
													 // all notes are
													 // processed
				  ( nFrameNumber < nPatternLengthInFrames ||
					pSampler->isRenderingNotes() ) ) ) {
			
			int nUsedBuffer = pDriver->m_nBufferSize;
			
			// This will calculate the size from -last- (end of
			// pattern) used frame buffer, which is mostly smaller
			// than pDriver->m_nBufferSize. But it only applies for
			// all patterns except of the last one. The latter we will
			// let ring until there is no further audio to process.
			if( patternPosition < nColumns - 1 &&
				nPatternLengthInFrames - nFrameNumber < pDriver->m_nBufferSize ){
				nLastRun = nPatternLengthInFrames - nFrameNumber;
				nUsedBuffer = nLastRun;
			};

			// Check whether the driver was stopped (since
			// AudioEngine::stopAudioDrivers() locks the audio engine
			// and the call to pDriver->m_processCallback) can not
			// acquire the lock).
			if ( ! pDriver->m_bIsRunning ) {
				__ERRORLOG( "Driver was stop before export was completed." );
				EventQueue::get_instance()->push_event( EVENT_PROGRESS, -1 );
				pDriver->m_bWritingFailed = true;
				tearDown();
				return nullptr;
			}
			
			int ret = pDriver->m_processCallback( nUsedBuffer, nullptr );

			// Only try a reasonable amount of times.
			int nMutexLockAttempts = 0;
			
			// In case the DiskWriter couldn't acquire the lock of the AudioEngine.
			while( ret == 2 ) {
				ret = pDriver->m_processCallback( nUsedBuffer, nullptr );

				// No need for a sleep() statement in here because the
				// AudioEngine::tryLockFor() in the processCallback
				// already introduces a delay.
				nMutexLockAttempts++;
				if ( nMutexLockAttempts > 30 ) {
					__ERRORLOG( "Too many attempts to lock the AudioEngine. Aborting." );
					
					EventQueue::get_instance()->push_event( EVENT_PROGRESS, -1 );
					pDriver->m_bWritingFailed = true;
					tearDown();
					return nullptr;
				}
			}

			if ( patternPosition == nColumns - 1 &&
				 nPatternLengthInFrames - nFrameNumber < nUsedBuffer ) {
				// The next buffer at least partially exceeds the song
				// size in ticks. As soon as it does we start to count
				// zeros in both audio channels. The moment we
				// encounter more than X we will stop the audio
				// export. Just waiting for the Sampler to finish
				// rendering is not sufficient because the Sample
				// itself can be zero padded at the end causing the
				// resulting .wav file to be inconsistent in terms of
				// length depending on the buffer sized use during
				// export.
				//
				// We are at the last pattern and just waited for the
				// Sampler to finish rendering all notes (at an
				// arbitrary point within the buffer).
				nBufferWriteLength = 0;

				int nSilentFrames = 0;
				for ( int ii = 0; ii < nUsedBuffer; ++ii ) {
					++nBufferWriteLength;
					
					if ( std::abs( pData_L[ii] ) == 0 &&
						 std::abs( pData_R[ii] ) == 0 ) {
						++nSuccessiveZeros;
					}

					if ( nSuccessiveZeros == nMaxNumberOfSilentFrames ) {
						break;
					}
				}
			} else {
				nBufferWriteLength = nUsedBuffer;
			}
			
			nFrameNumber += nBufferWriteLength;
			
			for ( unsigned ii = 0; ii < nBufferWriteLength; ii++ ) {
				if( pData_L[ ii ] > 1 ) {
					pData[ ii * 2 ] = 1;
				} else if( pData_L[ ii ] < -1 ) {
					pData[ ii * 2 ] = -1;
				} else {
					pData[ ii * 2 ] = pData_L[ ii ];
				}
				
				if( pData_R[ ii ] > 1 ){
					pData[ ii * 2 + 1 ] = 1;
				} else if ( pData_R[ ii ] < -1 ) {
					pData[ ii * 2 + 1 ] = -1;
				} else {
					pData[ ii * 2 + 1 ] = pData_R[ ii ];
				}
			}
			
			const int res = sf_writef_float( m_file, pData, nBufferWriteLength );
			if ( res != ( int )nBufferWriteLength ) {
				__ERRORLOG( QString( "Error during sf_write_float. Floats written: [%1], target: [%2]. %3" )
							.arg( res )
							.arg( nBufferWriteLength )
							.arg( sf_strerror( nullptr ) ) );

				EventQueue::get_instance()->push_event( EVENT_PROGRESS, -1 );
				pDriver->m_bWritingFailed = true;
				tearDown();
				return nullptr;
			}

			// Sampler is still rendering notes put we seem to have
			// reached the zero padding at the end of the
			// corresponding samples.
			if ( nSuccessiveZeros == nMaxNumberOfSilentFrames ) {
				break;
			}
		}
		
		// this progress bar method is not exact but ok enough to give users a usable visible progress feedback
		int nPercent = static_cast<int>( ( float )(patternPosition +1) /
										 ( float )nColumns * 100.0 );
		if ( nPercent < 100 ) {
			EventQueue::get_instance()->push_event( EVENT_PROGRESS, nPercent );
		}
	}

	// Explicitly mark export as finished.
	EventQueue::get_instance()->push_event( EVENT_PROGRESS, 100 );
	
	tearDown();
	return nullptr;
}



DiskWriterDriver::DiskWriterDriver( audioProcessCallback processCallback )
		: AudioOutput()
		, m_nSampleRate( 4800 )
		, m_nSampleDepth( 32 )
		, m_processCallback( processCallback )
		, m_nBufferSize( 1024 )
		, m_pOut_L( nullptr )
		, m_pOut_R( nullptr )
		, m_bIsRunning( false )
		, m_bDoneWriting( false )
		, m_bWritingFailed( false ) {
}



DiskWriterDriver::~DiskWriterDriver() {
}



int DiskWriterDriver::init( unsigned nBufferSize )
{
	INFOLOG( QString( "Init, buffer size: %1" ).arg( nBufferSize ) );

	m_nBufferSize = nBufferSize;
	
	m_pOut_L = new float[ m_nBufferSize ];
	m_pOut_R = new float[ m_nBufferSize ];

	return 0;
}

int DiskWriterDriver::connect()
{
	return 0;
}

void DiskWriterDriver::write()
{
	INFOLOG( "" );

	m_bIsRunning = true;
	
	pthread_attr_t attr;
	pthread_attr_init( &attr );

	pthread_create( &diskWriterDriverThread, &attr, diskWriterDriver_thread, this );
}

/// disconnect
void DiskWriterDriver::disconnect()
{
	INFOLOG( "" );
	
	m_bIsRunning = false;

	pthread_join( diskWriterDriverThread, nullptr );

	delete[] m_pOut_L;
	m_pOut_L = nullptr;

	delete[] m_pOut_R;
	m_pOut_R = nullptr;

}

unsigned DiskWriterDriver::getSampleRate()
{
	return m_nSampleRate;
}

QString DiskWriterDriver::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[DiskWriterDriver]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_nSampleRate: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nSampleRate ) )
			.append( QString( "%1%2m_sFilename: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sFilename ) )
			.append( QString( "%1%2m_nBufferSize: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nBufferSize ) )
			.append( QString( "%1%2m_nSampleDepth: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nSampleDepth ) )
			.append( QString( "%1%2m_bIsRunning: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bIsRunning ) )
			.append( QString( "%1%2m_bDoneWriting: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bDoneWriting ) )
			.append( QString( "%1%2m_bWritingFailed: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bWritingFailed ) );
	} else {
		sOutput = QString( "[DiskWriterDriver]" )
			.append( QString( " m_nSampleRate: %1" ).arg( m_nSampleRate ) )
			.append( QString( ", m_sFilename: %1" ).arg( m_sFilename ) )
			.append( QString( ", m_nBufferSize: %1" ).arg( m_nBufferSize ) )
			.append( QString( ", m_nSampleDepth: %1" ).arg( m_nSampleDepth ) )
			.append( QString( ", m_bIsRunning: %1" ).arg( m_bIsRunning ) )
			.append( QString( ", m_bDoneWriting: %1" ).arg( m_bDoneWriting ) )
			.append( QString( ", m_bWritingFailed: %1" ).arg( m_bWritingFailed ) );
	}

	return sOutput;
}
};
