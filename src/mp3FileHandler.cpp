////////////////////////////////Copyright © 2009////////////////////////////////
/*!
\file 		mp3FileHandler.h
\author 	Mike Beach
\brief		
*/
////////////////////////////////////////////////////////////////////////////////

//Includes
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include "mp3FileHandler.h"


////////////////////////////////////////////////////////////////////////////////
/*!
\brief		
\todo		Take parameters that can be used to override the default filename 
			formatting.
*/
////////////////////////////////////////////////////////////////////////////////
Mp3FileParser::Mp3FileParser( )
{
	GetDefaultFilenameInFmt( m_InFilenameFmt );
	GetDefaultFilenameOutFmt( m_OutFilenameFmt );
}


////////////////////////////////////////////////////////////////////////////////
/*!
\brief		
*/
////////////////////////////////////////////////////////////////////////////////
bool Mp3FileParser::AppendResolvedOutputFilename( const string& inFilePath, string& filenameOut, bool fromCompAlbum, const string& compAlbumArtist ) const
{
	const char* FILE_EXT = ".mp3";
	string fileAttributes[ NUM_ATTRIBUTES ];

	  // assert that this is a .mp3 file
	if( !IsMp3Filename(inFilePath) )
	{
		assert( IsMp3Filename(inFilePath) );
		return( false );
	}

	const FilenameComponentArray& inFileFmt = m_InFilenameFmt.first;
	const string& inDelimStr = m_InFilenameFmt.second;

	string filename = &inFilePath[ 0 ];
	size_t lastDirDelimIndx = inFilePath.find_last_of( '\\' );
	if( lastDirDelimIndx != string::npos && lastDirDelimIndx+1 < inFilePath.size() )
	{
		filename = &inFilePath[ lastDirDelimIndx+1 ];
	}

	  // parse the in filename
 	for( size_t strIndx = 0, fmtIndx = 0, delimIndx = 0; strIndx < filename.size() && fmtIndx < inFileFmt.size(); ++fmtIndx )
	{
		if( inFileFmt[fmtIndx] == DELIMITER_CHAR )
		{
			  // make sure the delimiters match
			if( filename[strIndx] != inDelimStr[delimIndx] )
			{
				assert( filename[strIndx] == inDelimStr[delimIndx] );
				return( false );
			}

			++delimIndx;
			++strIndx;
			continue;
		}

		string& attributeStr = fileAttributes[ inFileFmt[fmtIndx] ];

		  // make sure that this attribute hasn't been filled in yet
		assert( attributeStr.empty() );

		  // find where the string concerning this attribute ends
		size_t endStrIndx = string::npos;
		if( delimIndx >= inDelimStr.size() ) // if we've run out of delimiters
		{
			if( fmtIndx != inFileFmt.size() - 1 ) // assert that we're currently handling the last attribute 
			{
				assert( fmtIndx == inFileFmt.size() - 1 );
				return( false );
			}
			endStrIndx = filename.find( FILE_EXT, strIndx );
		}
		else
		{
			endStrIndx = filename.find( inDelimStr[delimIndx], strIndx );
		}

		if( endStrIndx == string::npos )
		{
			assert( endStrIndx != string::npos );
			return( false );
		}

		attributeStr = filename.substr( strIndx, endStrIndx - strIndx );

		strIndx = endStrIndx + 1;
		++delimIndx;
		++fmtIndx; // wanna skip the delim we just processed

		  // we need to make sure there are delimiters seperating our filename attributes (or else we can't tell them apart)
		if( fmtIndx < inFileFmt.size() && inFileFmt[fmtIndx] != DELIMITER_CHAR )
		{
			assert( fmtIndx >= inFileFmt.size() || inFileFmt[fmtIndx] == DELIMITER_CHAR );
			return( false );
		}
	}

	  // if we're still missing some attribute or we couldn't "normalize" the track number
	if( !ParseFileForMissingAttributes(inFilePath, fileAttributes, fromCompAlbum, compAlbumArtist) || !NormalizeTrackNumber(fileAttributes[TRACK_NUMBER]) ) 
	{
		return( false ); // we can't construct a proper output file path
	}
	
	const FilenameComponentArray& outFileFmt = m_OutFilenameFmt.first;
	const string& outDelimStr = m_OutFilenameFmt.second;

	  // construct the out filename
	for( unsigned fmtIndx = 0, delimIndx = 0; fmtIndx < outFileFmt.size(); ++fmtIndx )
	{
		if( outFileFmt[fmtIndx] == DELIMITER_CHAR )
		{
			filenameOut += outDelimStr[ delimIndx ];

			++delimIndx;
			continue;
		}

		string& attributeStr = fileAttributes[ outFileFmt[fmtIndx] ];

		  // make sure that this attribute was filled in
		if( attributeStr.empty() )
		{
			assert( !attributeStr.empty() );
			return( false );
		}

		  // if this string is invalid for file naming 
		if( !IsValidForFilename(attributeStr) )
		{
			return( false );
		}
		filenameOut += attributeStr;
	}

	filenameOut += FILE_EXT;
	return( IsValidFilePath(filenameOut) );
}


////////////////////////////////////////////////////////////////////////////////
/*!
\brief		
*/
////////////////////////////////////////////////////////////////////////////////
bool Mp3FileParser::IsValidFilePath( const string& filePath ) const
{
	const char DRIVE_DELIMITER = ':';
	size_t driveDelimIndx = filePath.find( DRIVE_DELIMITER );
	if( driveDelimIndx != string::npos ) // if we found a drive delimiter in the specified path
	{
		  // if the drive delimiter wasn't where it is supposed to be in the filepath (say in a directory or filename)
		if( driveDelimIndx != 1 || driveDelimIndx+1 >= filePath.size() || (filePath[driveDelimIndx+1] != '\\' && filePath[driveDelimIndx+1] != '/') )
		{
			return( false );
		}
	}

	const string INVALID_FILE_PATH_CHARS = "*?\"<>|"; // characters that are invalid for directory and file naming, except for slashes (\ or /) which can be used in file paths
	for( unsigned charIndx = 0; charIndx < INVALID_FILE_PATH_CHARS.size(); ++charIndx )
	{
		if( filePath.find( INVALID_FILE_PATH_CHARS[charIndx] ) != string::npos )
		{
			return( false );
		}
	}

	return( true );
}


////////////////////////////////////////////////////////////////////////////////
/*!
\brief		
*/
////////////////////////////////////////////////////////////////////////////////
bool Mp3FileParser::IsValidForFilename( const string& filename ) const
{
	const string PATH_DELIMITER_CHARS = ":\\/"; // characters that are invalid for filenames, but vaild for file paths (:, \, and /, which are used as drive/directory delimiters)
	for( unsigned charIndx = 0; charIndx < PATH_DELIMITER_CHARS.size(); ++charIndx )
	{
		if( filename.find( PATH_DELIMITER_CHARS[charIndx] ) != string::npos )
		{
			return( false );
		}
	}

	  // now that we've checked that the proposed filename is free of drive/directory delimiters, check that is doesn't contain any other invalid characters (invalid for directories too)
	return( IsValidFilePath(filename) ); 
}

////////////////////////////////////////////////////////////////////////////////
/*!
\brief		
*/
////////////////////////////////////////////////////////////////////////////////
bool Mp3FileParser::IsMp3Filename( const string& inFilename )
{
	const char* MP3_FILE_EXTENSION = ".mp3";

	size_t extLen = strlen( MP3_FILE_EXTENSION );
	size_t subStrIndx = inFilename.find( MP3_FILE_EXTENSION );

	
	while( subStrIndx != string::npos )
	{
		if( subStrIndx == inFilename.size() - extLen )
		{
			return( true );
		}

		subStrIndx = inFilename.find( MP3_FILE_EXTENSION, subStrIndx + extLen );
	}

	return( false );
}

////////////////////////////////////////////////////////////////////////////////
/*!
\brief		
*/
////////////////////////////////////////////////////////////////////////////////
bool Mp3FileParser::AlphabetizeArtistName( const string& artistIn, string& alpabetizedOut ) const
{
	const string DETERMINER_PHRASE( "The" );

	if( artistIn.size() > DETERMINER_PHRASE.size() && artistIn[ DETERMINER_PHRASE.size() ] == ' ' )
	{
		string lowerCaseDeterminer		= DETERMINER_PHRASE;
		string lowerCaseArtistSubStr	= artistIn.substr( 0, lowerCaseDeterminer.size() );
		  // need to make sure that we're comparing the same character set (upper case vs. lower case)
		for( unsigned phraseIndx = 0; phraseIndx < lowerCaseDeterminer.size(); ++phraseIndx )
		{
			lowerCaseDeterminer[ phraseIndx ]	= _tolower( lowerCaseDeterminer[phraseIndx] );
			lowerCaseArtistSubStr[ phraseIndx ] = _tolower( lowerCaseArtistSubStr[phraseIndx] );
		}

		if( !lowerCaseDeterminer.compare(lowerCaseArtistSubStr) )
		{
			if( artistIn.size() <= DETERMINER_PHRASE.size()+1 ) 
			{
				assert( artistIn.size() > DETERMINER_PHRASE.size()+1 ); // make sure there's more to the artist's name after the space
				return( false );
			}

			alpabetizedOut = artistIn.substr( DETERMINER_PHRASE.size()+1 );
			alpabetizedOut += ", ";
			alpabetizedOut += DETERMINER_PHRASE;

			return( true );
		}
	}

	alpabetizedOut = artistIn;
	return( true );
}


////////////////////////////////////////////////////////////////////////////////
/*!
\brief		
*/
////////////////////////////////////////////////////////////////////////////////
bool Mp3FileParser::NormalizeTrackNumber( string& trackNo ) const
{
	if( trackNo.size() > 1 )
	{
		return( true );
	}

	if( trackNo.empty() )
	{
		assert( !trackNo.empty() );
		return( false );
	}

	trackNo.insert( 0, "0" );
	return( true );
}


////////////////////////////////////////////////////////////////////////////////
/*!
\brief		
*/
////////////////////////////////////////////////////////////////////////////////
bool Mp3FileParser::ParseFileForMissingAttributes( const string& fullFilePath, string attribStrs[NUM_ATTRIBUTES], bool fromCompAlbum, const string& compAlbumArtist ) const
{
	FILE* pFile = 0;

	for( unsigned atrribIndx = 0; atrribIndx < NUM_ATTRIBUTES; ++atrribIndx )
	{
		if( !attribStrs[atrribIndx].empty() )
		{
			continue;
		}

		switch( atrribIndx )
		{
		case ALPHABETIZED_ALBUM_ARTIST:
			{
				const string* pAlbumArtist = &attribStrs[ARTIST];
				if( fromCompAlbum )
				{
					pAlbumArtist = &compAlbumArtist;
				}

				if( pAlbumArtist->empty() ) // might happen if ARTIST < ALPHABETIZED_ALBUM_ARTIST... make sure that this isn't the case
				{
					assert( !pAlbumArtist->empty() ); 
					return( false );
				}

				if( !AlphabetizeArtistName(*pAlbumArtist, attribStrs[ALPHABETIZED_ALBUM_ARTIST]) )
				{
					return( false );
				}
			}
			break;

		case RELEASE_YEAR: 
			{
				if( !pFile ) // if the file hasn't been opened yet
				{
					fopen_s( &pFile, fullFilePath.c_str(), "rb" );
					assert( pFile ); // make sure that it was opened
				}
				if( !ParseMp3ForId3v2Year(pFile, attribStrs[atrribIndx]) )
				{
					return( false );
				}
			}
			break;

		default:
			{
				assert( atrribIndx == RELEASE_YEAR || atrribIndx == ALPHABETIZED_ALBUM_ARTIST ); // at this point in time we only support grabbing of the album's release year from the file

				if( pFile )
				{
					fclose( pFile );
				}
				return( false );
			}

		}; // switch( atrribIndx )
	}

	if( pFile )
	{
		fclose( pFile );
	}
	return( true );
}


////////////////////////////////////////////////////////////////////////////////
/*!
\brief		
*/
////////////////////////////////////////////////////////////////////////////////
bool IsLittleEndianMachine( )
{
	long int testInt = 0x12345678;
	char* pMem = (char*)&testInt;

	return( pMem[0] == 0x78 );
}

////////////////////////////////////////////////////////////////////////////////
/*!
\brief		Assumes the number being passed in is big endian.
*/
////////////////////////////////////////////////////////////////////////////////
template< typename T >
void SwapFromBigEndianness( T& mirrorMe )
{
	unsigned operandSize = sizeof( mirrorMe );
	if( IsLittleEndianMachine() && operandSize > 1 )
	{
		assert( !(operandSize % 2) ); // assert that there's an even number of bytes that we're mirroring (the below method won't work other wise)

		for( unsigned hiByteIndx = 0; hiByteIndx < operandSize / 2; ++hiByteIndx )
		{
			char* byteArray = (char*)&mirrorMe;

			unsigned reflectedLoByteIndx = operandSize - hiByteIndx - 1;
			
			  // swap the bytes around
			char newLoBits = byteArray[ hiByteIndx ];
			byteArray[ hiByteIndx ] = byteArray[ reflectedLoByteIndx ];
			byteArray[ reflectedLoByteIndx ] = newLoBits;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
/*!
\brief		Note: That this function was based on ID3v2 formatting described 
			here: http://www.id3.org/id3v2.3.0.
*/
////////////////////////////////////////////////////////////////////////////////
bool Mp3FileParser::ParseMp3ForId3v2Year( FILE* pFile, string& id3YearOut ) const
{
	if( !pFile )
	{
		assert( pFile );
		return( false );
	}

	const unsigned ID3_ID_SIZE = 3;
	char id3TagId[ ID3_ID_SIZE ];
	fread( id3TagId, sizeof(char), ID3_ID_SIZE, pFile );

	  // make sure that this mp3 has an ID3v2 tag
	const char ID3v2_IDENTIFIER[ ID3_ID_SIZE ] = { 'I','D','3' };
	if( memcmp(ID3v2_IDENTIFIER, id3TagId, ID3_ID_SIZE) )
	{
		assert( !memcmp(ID3v2_IDENTIFIER, id3TagId, ID3_ID_SIZE) );
		return( false );
	}

	  // move past data we don't care about (versioning info)
	const unsigned ID3_FLAGS_OFFSET_FROM_TAG_ID = 2;
	fseek( pFile, ID3_FLAGS_OFFSET_FROM_TAG_ID, SEEK_CUR );

	  // need to check the flags and see if there is an extended header stuck in here
	char id3Flags;
	fread( &id3Flags, sizeof(id3Flags), 1, pFile );

	  // if any of the unused bits are set then we may be dealing with a corrupted mp3 file
	const char UNUSED_FLAG_BITS = ~( 1<<7 | 1<<6 | 1<<5 );
	if( id3Flags & UNUSED_FLAG_BITS )
	{
		assert( !(id3Flags & UNUSED_FLAG_BITS) );
		return( false );
	}

	const unsigned ID3_SIZE_SIZE = 4;
	unsigned char id3SizeBytes[ ID3_SIZE_SIZE ] = { 0 };
	fread( &id3SizeBytes, sizeof(char), ID3_SIZE_SIZE, pFile );

	int id3Size = 0;
	assert( sizeof(id3Size) == ID3_SIZE_SIZE );
	  // the top bit for each byte denoting size is unused, so we need to properly calculate the size
	for( unsigned byteNum = 0, byteIndx = ID3_SIZE_SIZE-1; byteNum < ID3_SIZE_SIZE; ++byteNum, --byteIndx )
	{
		unsigned char hiBits = 0;
		if( byteNum+1 < ID3_SIZE_SIZE )
		{
			hiBits = id3SizeBytes[ byteIndx-1 ] << ( 7 - byteNum );
			id3SizeBytes[ byteIndx-1 ] >>= byteNum + 1; // get rid of the lo bits that went to the this current byte
		}

		unsigned sizeOperand  = id3SizeBytes[ byteIndx ] | hiBits;
		sizeOperand <<= 8*byteNum;

		id3Size += sizeOperand;
	}

	  // if there is an extended header then let's push past it and move on
	const char EXTENDED_HEADER_FLAG = (1<<6);
	if( id3Flags & EXTENDED_HEADER_FLAG )
	{
		const unsigned EXTENDED_HEADER_SIZE = 10;
		id3Size -= EXTENDED_HEADER_SIZE; // the size counts the 

		const unsigned ID3_EXTENDED_HEADER_FLAGS_OFFSET = 4;
		fseek( pFile, ID3_EXTENDED_HEADER_FLAGS_OFFSET, SEEK_CUR );

		char id3HiExtendedFlags;
		fread( &id3HiExtendedFlags, sizeof(id3HiExtendedFlags), 1, pFile );

		  // now push past the extended header, we don't care about anything else in it
		fseek( pFile, EXTENDED_HEADER_SIZE - ID3_EXTENDED_HEADER_FLAGS_OFFSET, SEEK_CUR );

		const char EXTENDED_HEADER_CRC_DATA_FLAG = (1<<7);
		if( id3HiExtendedFlags & ~EXTENDED_HEADER_CRC_DATA_FLAG ) // only the msb is used, any other bits that are set might signify a corrupted file
		{
			assert( !(id3HiExtendedFlags & ~EXTENDED_HEADER_CRC_DATA_FLAG) ); 
			return( false );
		}

		  // if this flag was set, then it means there's another 4 bytes to blow past
		if( id3HiExtendedFlags & EXTENDED_HEADER_CRC_DATA_FLAG )
		{
			const unsigned CRC_DATA_SIZE = 4;
			id3Size -= CRC_DATA_SIZE;

			fseek( pFile, CRC_DATA_SIZE, SEEK_CUR );
		}
	}

	  // 
	  // now we're past all the header crap, time to parse along, searching for the frame we want

	while( id3Size > 0 )
	{
		const unsigned ID3_FRAME_ID_SIZE = 4;
		char id3FrameId[ ID3_FRAME_ID_SIZE+1 ] = {0};
		fread( id3FrameId, sizeof(char), ID3_FRAME_ID_SIZE, pFile );

		unsigned id3FrameSize = 0;
		assert( sizeof(id3FrameSize) == 4 ); // make sure that the buffer we're trying to fill is the right size (the frame's size is 4 bytes long)
		fread( &id3FrameSize, sizeof(id3FrameSize), 1, pFile );
		  // if this is a little endian machine we need to mirror the frame's size bytes that we just read in
		SwapFromBigEndianness( id3FrameSize );
		
		const char ID3v2_YEAR_FRAME_ID[ ID3_FRAME_ID_SIZE ] = { 'T','Y','E','R' };
		if( !memcmp(ID3v2_YEAR_FRAME_ID, id3FrameId, ID3_FRAME_ID_SIZE) )
		{
			unsigned short id3FrameFlags = 0;
			assert( sizeof(id3FrameFlags) == 2 ); // make sure that the buffer we're trying to fill is the right size (the frame's flags are 2 bytes long)
			fread( &id3FrameFlags, sizeof(id3FrameFlags), 1, pFile );
			  // if this is a little endian machine we need to mirror the frame's flag bytes that we just read in
			SwapFromBigEndianness( id3FrameFlags );

			const unsigned short UNSUPPORTED_FRAME_FLAGS = (1<<7) | (1<<6); // we currently don't support frame compression and encryption
			if( id3FrameFlags & UNSUPPORTED_FRAME_FLAGS )
			{
				assert( !(id3FrameFlags & UNSUPPORTED_FRAME_FLAGS) );
				return( false );
			}

			bool isTexEncodedFrame = true;// already know this is true (id3FrameId[0] == 'T') && strcmp( id3FrameId, "TXXX" );
			if( isTexEncodedFrame )
			{
				char stringEncoding;
				fread( &stringEncoding, sizeof(stringEncoding), 1, pFile );

				const char EXPECTED_STRING_ENCODING = 0x00;
				if( stringEncoding != EXPECTED_STRING_ENCODING ) // at this time we don't support unicode, only ISO-8859-1
				{
					assert( stringEncoding == EXPECTED_STRING_ENCODING ); 
					return( false );
				}

				--id3FrameSize;
			}

			const unsigned ID3_FRAME_EXPECTED_DATA_SIZE = 4;
			if( id3FrameSize < ID3_FRAME_EXPECTED_DATA_SIZE ) // if the year is less than 4 characters then something is wrong
			{
				assert( id3FrameSize >= ID3_FRAME_EXPECTED_DATA_SIZE );
				return( false );
			}

			  // read in the mp3's release year
			char id3FrameData[ ID3_FRAME_EXPECTED_DATA_SIZE+1 ] = { 0 };
			fread( id3FrameData, sizeof(char), ID3_FRAME_EXPECTED_DATA_SIZE, pFile );

			if( id3FrameSize > ID3_FRAME_EXPECTED_DATA_SIZE )
			{
				char id3FrameStringTerminationChar;
				fread( &id3FrameStringTerminationChar, sizeof(id3FrameStringTerminationChar), 1, pFile );

				char EXPECTED_TERMINATION_CHAR = 0x00;

				if( id3FrameStringTerminationChar != EXPECTED_TERMINATION_CHAR ) // if the string is longer than we expected and the next char isn't terminating the string... then something is wrong
				{ 
					assert( id3FrameStringTerminationChar == EXPECTED_TERMINATION_CHAR ); 
					return( false );
				}
			}

			id3YearOut = id3FrameData;
			return( true ); // return, we got what we needed!
		}

		const unsigned REMAINING_FRAME_HEADER_SIZE = 2;
		  // move onto the next frame		
		fseek( pFile, REMAINING_FRAME_HEADER_SIZE + id3FrameSize, SEEK_CUR );

		id3Size -= ID3_FRAME_ID_SIZE + sizeof(id3FrameSize) + REMAINING_FRAME_HEADER_SIZE + id3FrameSize;
	};

	assert( id3Size <= 0 ); // didn't find what we were looking for
	return( false );
}


////////////////////////////////////////////////////////////////////////////////
/*!
\brief		Specifies the input file's expected formatting (if one wasn't 
			specified), "Artist_Album_TrackNumber_SongTitle.mp3".
*/
////////////////////////////////////////////////////////////////////////////////
void Mp3FileParser::GetDefaultFilenameInFmt( FilenameFmt& fmtOut )
{
	FilenameComponentArray& filenameComponents = fmtOut.first;
	string& delimiterString = fmtOut.second;

	filenameComponents.clear();
	delimiterString.clear();

	const char DEFAULT_IN_FILENAME_DELIM = '_';

	filenameComponents.push_back( ARTIST );
	filenameComponents.push_back( DELIMITER_CHAR ); delimiterString.push_back( DEFAULT_IN_FILENAME_DELIM );
	filenameComponents.push_back( ALBUM );
	filenameComponents.push_back( DELIMITER_CHAR ); delimiterString.push_back( DEFAULT_IN_FILENAME_DELIM );
	filenameComponents.push_back( TRACK_NUMBER );
	filenameComponents.push_back( DELIMITER_CHAR ); delimiterString.push_back( DEFAULT_IN_FILENAME_DELIM );
	filenameComponents.push_back( SONG_TITLE );
}


////////////////////////////////////////////////////////////////////////////////
/*!
\brief		Specifies the output file's naming format, "Artist (ReleaseYear) - 
			Album\Artist - Album - TrackNumber - SongTitle.mp3".
*/
////////////////////////////////////////////////////////////////////////////////
void Mp3FileParser::GetDefaultFilenameOutFmt( FilenameFmt& fmtOut )
{
	FilenameComponentArray& filenameComponents = fmtOut.first;
	string& delimiterString = fmtOut.second;

	filenameComponents.clear();
	delimiterString.clear();

	  // sub-folder name
	filenameComponents.push_back( ALPHABETIZED_ALBUM_ARTIST );
	filenameComponents.push_back( DELIMITER_CHAR ); delimiterString.push_back( ' ' );
	filenameComponents.push_back( DELIMITER_CHAR ); delimiterString.push_back( '(' );
	filenameComponents.push_back( RELEASE_YEAR );
	filenameComponents.push_back( DELIMITER_CHAR ); delimiterString.push_back( ')' );
	filenameComponents.push_back( DELIMITER_CHAR ); delimiterString.push_back( ' ' );
	filenameComponents.push_back( DELIMITER_CHAR ); delimiterString.push_back( '-' );
	filenameComponents.push_back( DELIMITER_CHAR ); delimiterString.push_back( ' ' );
	filenameComponents.push_back( ALBUM );
	filenameComponents.push_back( DELIMITER_CHAR ); delimiterString.push_back( '\\' );
	  // filename
	filenameComponents.push_back( ARTIST );
	filenameComponents.push_back( DELIMITER_CHAR ); delimiterString.push_back( ' ' );
	filenameComponents.push_back( DELIMITER_CHAR ); delimiterString.push_back( '-' );
	filenameComponents.push_back( DELIMITER_CHAR ); delimiterString.push_back( ' ' );
	filenameComponents.push_back( ALBUM );
	filenameComponents.push_back( DELIMITER_CHAR ); delimiterString.push_back( ' ' );
	filenameComponents.push_back( DELIMITER_CHAR ); delimiterString.push_back( '-' );
	filenameComponents.push_back( DELIMITER_CHAR ); delimiterString.push_back( ' ' );
	filenameComponents.push_back( TRACK_NUMBER );
	filenameComponents.push_back( DELIMITER_CHAR ); delimiterString.push_back( ' ' );
	filenameComponents.push_back( DELIMITER_CHAR ); delimiterString.push_back( '-' );
	filenameComponents.push_back( DELIMITER_CHAR ); delimiterString.push_back( ' ' );
	filenameComponents.push_back( SONG_TITLE );
}