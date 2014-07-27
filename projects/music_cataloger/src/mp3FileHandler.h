////////////////////////////////Copyright © 2009////////////////////////////////
/*!
\file 		mp3FileHandler.h
\author 	Mike Beach
\brief		
*/
////////////////////////////////////////////////////////////////////////////////

//Includes
#include <string>
#include <vector>

//Using
using std::string;
using std::vector;
using std::pair;


class Mp3FileParser
{
public: // member functions

	Mp3FileParser();

	bool AppendResolvedOutputFilename( const string& inFilePath, string& filenameOut, bool fromCompAlbum = false, const string& compAlbumArtist = "Various Artists" ) const;
	static bool IsMp3Filename( const string& inFilename );

private: // constants

	enum FileNamingAttributes
	{
		SONG_TITLE = 0,
		ARTIST,
		ALPHABETIZED_ALBUM_ARTIST,
		ALBUM,
		TRACK_NUMBER,
		RELEASE_YEAR,

		NUM_ATTRIBUTES
	};

	  // kind-of-a hack... we're using FileNamingAttributes in two different ways: to index into an array, and to define the filename formatting (thus the need for this "DELIMITER_CHAR")
	static const FileNamingAttributes DELIMITER_CHAR = NUM_ATTRIBUTES; 

private: // typedefs

	typedef vector< FileNamingAttributes >			FilenameComponentArray;
	typedef pair< FilenameComponentArray, string >	FilenameFmt;

private: // member functions

	void GetDefaultFilenameInFmt( FilenameFmt& fmtOut );
	void GetDefaultFilenameOutFmt( FilenameFmt& fmtOut );
	bool AlphabetizeArtistName( const string& artistIn, string& alpabetizedOut ) const;
	bool NormalizeTrackNumber( string& trackNo ) const;
	bool ParseFileForMissingAttributes( const string& fullFilePath, string attribStrs[NUM_ATTRIBUTES], bool fromCompAlbum = false, const string& compAlbumArtist = "Various Artists" ) const;
	bool ParseMp3ForId3v2Year( FILE* pFile, string& id3YearOut ) const;
	bool IsValidFilePath( const string& filePath ) const;
	bool IsValidForFilename( const string& filename ) const;

private: // member variables

	FilenameFmt m_InFilenameFmt;
	FilenameFmt m_OutFilenameFmt;
	bool		m_IsFromACompilationAlbum;
	bool		m_UseAlbumNameAsAlbumArtist;
};