/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#include <algorithm>

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

	DiskWriterDriver *pDriver = ( DiskWriterDriver* )param;

	EventQueue::get_instance()->pushEvent( Event::Type::Progress, 0 );

	auto pAudioEngine = Hydrogen::get_instance()->getAudioEngine();
	
	___INFOLOG( "DiskWriterDriver thread started" );

	const auto format = Filesystem::AudioFormatFromSuffix( pDriver->m_sFileName );

	SF_INFO soundInfo;
	soundInfo.samplerate = pDriver->m_nSampleRate;
	soundInfo.channels = 2;

	// default format
	int sfformat = SF_FORMAT_WAV; //wav format (default)
	int bits = SF_FORMAT_PCM_16; //16 bit PCM (default)

	// Determine audio format based on the provided file suffix.
	if ( format == Filesystem::AudioFormat::Aiff ||
		 format == Filesystem::AudioFormat::Aifc ||
		 format == Filesystem::AudioFormat::Aif ) {
		sfformat =  SF_FORMAT_AIFF;
	}
#ifdef H2CORE_HAVE_FLAC_SUPPORT
	else if ( format == Filesystem::AudioFormat::Flac ) {
		sfformat =  SF_FORMAT_FLAC;
	}
#endif
	else if ( format == Filesystem::AudioFormat::Wav ) {
		sfformat =  SF_FORMAT_WAV;
	}
	else if ( format == Filesystem::AudioFormat::Au ) {
		sfformat =  SF_FORMAT_AU;
	}
	else if ( format == Filesystem::AudioFormat::Caf ) {
		sfformat =  SF_FORMAT_CAF;
	}
	else if ( format == Filesystem::AudioFormat::W64 ) {
		sfformat =  SF_FORMAT_W64;
	}
#ifdef H2CORE_HAVE_FLAC_SUPPORT
	else if ( format == Filesystem::AudioFormat::Ogg ) {
		sfformat = SF_FORMAT_OGG;
		bits = SF_FORMAT_VORBIS;
	}
#endif
#ifdef H2CORE_HAVE_OPUS_SUPPORT
	else if ( format == Filesystem::AudioFormat::Opus ) {
		sfformat = SF_FORMAT_OGG;
		bits = SF_FORMAT_OPUS;
	}
#endif
	else if ( format == Filesystem::AudioFormat::Voc ) {
		sfformat =  SF_FORMAT_VOC;
	}
#ifdef H2CORE_HAVE_MP3_SUPPORT
	else if ( format == Filesystem::AudioFormat::Mp3 ) {
		sfformat =  SF_FORMAT_MPEG;
		bits = SF_FORMAT_MPEG_LAYER_III;
	}
#endif
	else {
		___ERRORLOG( QString( "Unsupported file extension [%1] using libsndfile [%2]" )
					.arg( pDriver->m_sFileName ).arg( sf_version_string() ) );
		pDriver->m_bDoneWriting = true;
		pDriver->m_bWritingFailed = true;
		EventQueue::get_instance()->pushEvent( Event::Type::Progress, 100 );
		pthread_exit( nullptr );
		return nullptr;

	}

	// Instead of making audio export fail on non-supported parameter
	// combinations, we tailor this test and UI to only allow valid ones. It
	// would be bad UX to provide an invalid option.

	if ( format != Filesystem::AudioFormat::Ogg &&
		 format != Filesystem::AudioFormat::Opus &&
		 format != Filesystem::AudioFormat::Mp3 ) {
		// Handle sample depth
		if ( pDriver->m_nSampleDepth == 8 ) {
			// WAV and other raw PCM formats are handled differently.
			if ( format == Filesystem::AudioFormat::Voc ||
				 format == Filesystem::AudioFormat::W64 ||
				 format == Filesystem::AudioFormat::Wav ) {
				bits = SF_FORMAT_PCM_U8; //Unsigned 8 bit data needed for Microsoft WAV format
			} else {
				bits = SF_FORMAT_PCM_S8; //Signed 8 bit data works with aiff
			}
		}
		else if ( pDriver->m_nSampleDepth == 16 ) {
			bits = SF_FORMAT_PCM_16; //Signed 16 bit data
		}
		else if ( pDriver->m_nSampleDepth == 24 ) {
			bits = SF_FORMAT_PCM_24; //Signed 24 bit data
		}
		else if ( pDriver->m_nSampleDepth == 32 ) {
			bits = SF_FORMAT_PCM_32; ////Signed 32 bit data
		}
	}

	soundInfo.format =  sfformat|bits;

	if ( !sf_format_check( &soundInfo ) ) {
		___ERRORLOG( QString( "Error while checking format using libsndfile [%1]" )
					.arg( sf_version_string() ) );
		pDriver->m_bDoneWriting = true;
		pDriver->m_bWritingFailed = true;
		EventQueue::get_instance()->pushEvent( Event::Type::Progress, 100 );
		pthread_exit( nullptr );
		return nullptr;
	}

#ifdef WIN32
	// On Windows we use a special version of sf_open to ensure we get all
	// characters of the filename entered in the GUI right. No matter which
	// encoding was used locally.
	// We have to terminate the string using a null character ourselves.
	QString sPaddedPath = pDriver->m_sFileName.append( '\0' );
	wchar_t* encodedFileName = new wchar_t[ sPaddedPath.size() ];

	sPaddedPath.toWCharArray( encodedFileName );
	
	SNDFILE* pSndfile = sf_wchar_open( encodedFileName, SFM_WRITE,
								   &soundInfo );
	delete encodedFileName;
#else
	SNDFILE* pSndfile = sf_open( pDriver->m_sFileName.toLocal8Bit(), SFM_WRITE,
							   &soundInfo );
#endif

	if ( pSndfile == nullptr ) {
		___ERRORLOG( QString( "Unable to open file [%1] with format [%2] using libsndfile [%3]: %4" )
					.arg( pDriver->m_sFileName )
					.arg( Sample::sndfileFormatToQString( soundInfo.format ) )
					.arg( sf_version_string() )
					.arg( sf_strerror( pSndfile ) ) );
		pDriver->m_bDoneWriting = true;
		pDriver->m_bWritingFailed = true;
		EventQueue::get_instance()->pushEvent( Event::Type::Progress, 100 );
		pthread_exit( nullptr );
		return nullptr;
	}

	// Perform some per-file settings.
#ifdef H2CORE_HAVE_MP3_SUPPORT
	if ( format == Filesystem::AudioFormat::Mp3 ) {
		int nBitrateMode = SF_BITRATE_MODE_VARIABLE;
		if ( sf_command( pSndfile, SFC_SET_BITRATE_MODE, &nBitrateMode,
						 sizeof(int) ) != SF_TRUE ) {
			___WARNINGLOG( QString( "Unable to set variable bitrate for MP3 encoding: %1" )
						  .arg( sf_strerror( pSndfile ) ) );
		}
	}
#endif

#ifdef H2CORE_HAVE_FLAC_SUPPORT
	// FLAC (and OGG/Vorbis) is the oldest format supporting this setting.
	if ( format == Filesystem::AudioFormat::Mp3 ||
		 format == Filesystem::AudioFormat::Ogg ||
		 format == Filesystem::AudioFormat::Opus ||
		 format == Filesystem::AudioFormat::Flac ) {
		if ( sf_command( pSndfile, SFC_SET_COMPRESSION_LEVEL,
						 &pDriver->m_fCompressionLevel,
						 sizeof(double) ) != SF_TRUE ) {
			___WARNINGLOG( QString( "Unable to set compression level [%1]: %2" )
						  .arg( pDriver->m_fCompressionLevel )
						  .arg( sf_strerror( pSndfile ) ) );
		}
	}
#endif

	float *pData = new float[ pDriver->m_nBufferSize * 2 ];	// always stereo

	float *pData_L = pDriver->m_pOut_L;
	float *pData_R = pDriver->m_pOut_R;

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pSampler = pHydrogen->getAudioEngine()->getSampler();

	// always rolling, no user interaction
	pAudioEngine->play();

	auto pPatternColumns = pSong->getPatternGroupVector();
	int nColumns = pPatternColumns->size();

	// Used to cleanly terminate this thread and close all handlers.
	auto tearDown = [&](){
		pDriver->m_bDoneWriting = true;
		delete[] pData;
		pData = nullptr;

		sf_close( pSndfile );

		___INFOLOG( "DiskWriterDriver thread end" );

		pthread_exit( nullptr );
	};
	
	int nPatternSize, nBufferWriteLength;
	float fBpm;
	float fTicksize = 0;
	int nMaxNumberOfSilentFrames = 200;
	for ( int patternPosition = 0; patternPosition < nColumns; ++patternPosition ) {
		
		auto pColumn = ( *pPatternColumns )[ patternPosition ];
		if ( pColumn->size() != 0 ) {
			nPatternSize = pColumn->longestPatternLength();
		} else {
			nPatternSize = 4 * H2Core::nTicksPerQuarter;
		}

		fBpm = AudioEngine::getBpmAtColumn( patternPosition );
		fTicksize = AudioEngine::computeTickSize( pDriver->m_nSampleRate, fBpm );
		
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
				___ERRORLOG( "Driver was stop before export was completed." );
				EventQueue::get_instance()->pushEvent( Event::Type::Progress, -1 );
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
					___ERRORLOG( "Too many attempts to lock the AudioEngine. Aborting." );
					
					EventQueue::get_instance()->pushEvent( Event::Type::Progress, -1 );
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
			
			const int res = sf_writef_float( pSndfile, pData, nBufferWriteLength );
			if ( res != ( int )nBufferWriteLength ) {
				___ERRORLOG( QString( "Error during sf_write_float using [%1]. Floats written: [%2], target: [%3]. %4" )
							.arg( sf_version_string() ).arg( res )
							.arg( nBufferWriteLength )
							.arg( sf_strerror( nullptr ) ) );

				EventQueue::get_instance()->pushEvent( Event::Type::Progress, -1 );
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
			EventQueue::get_instance()->pushEvent( Event::Type::Progress, nPercent );
		}
	}

	// Explicitly mark export as finished.
	EventQueue::get_instance()->pushEvent( Event::Type::Progress, 100 );
	
	tearDown();

	return nullptr;
}



DiskWriterDriver::DiskWriterDriver( audioProcessCallback processCallback )
		: AudioDriver()
		, m_nSampleRate( 4800 )
		, m_nSampleDepth( 32 )
		, m_processCallback( processCallback )
		, m_nBufferSize( 1024 )
		, m_pOut_L( nullptr )
		, m_pOut_R( nullptr )
		, m_bIsRunning( false )
		, m_bDoneWriting( false )
		, m_bWritingFailed( false )
		, m_fCompressionLevel( 0.0 ) {
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
			.append( QString( "%1%2m_sFileName: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sFileName ) )
			.append( QString( "%1%2m_nBufferSize: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nBufferSize ) )
			.append( QString( "%1%2m_nSampleDepth: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nSampleDepth ) )
			.append( QString( "%1%2m_bIsRunning: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bIsRunning ) )
			.append( QString( "%1%2m_bDoneWriting: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bDoneWriting ) )
			.append( QString( "%1%2m_bWritingFailed: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bWritingFailed ) )
			.append( QString( "%1%2m_fCompressionLevel: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fCompressionLevel ) );
	} else {
		sOutput = QString( "[DiskWriterDriver]" )
			.append( QString( " m_nSampleRate: %1" ).arg( m_nSampleRate ) )
			.append( QString( ", m_sFileName: %1" ).arg( m_sFileName ) )
			.append( QString( ", m_nBufferSize: %1" ).arg( m_nBufferSize ) )
			.append( QString( ", m_nSampleDepth: %1" ).arg( m_nSampleDepth ) )
			.append( QString( ", m_bIsRunning: %1" ).arg( m_bIsRunning ) )
			.append( QString( ", m_bDoneWriting: %1" ).arg( m_bDoneWriting ) )
			.append( QString( ", m_bWritingFailed: %1" ).arg( m_bWritingFailed ) )
			.append( QString( ", m_fCompressionLevel: %1" )
					 .arg( m_fCompressionLevel ) );
	}

	return sOutput;
}

void DiskWriterDriver::setCompressionLevel( double fCompressionLevel ) {
	if ( fCompressionLevel > 1.0 || fCompressionLevel < 0.0 ) {
		ERRORLOG( QString( "Provided compression level [%1] out of bound [0.0, 1.0]. Assigning nearest possible value." )
				  .arg( fCompressionLevel ) );
		fCompressionLevel = std::clamp( fCompressionLevel, 0.0, 1.0 );
	}

	m_fCompressionLevel = fCompressionLevel;
}
};
