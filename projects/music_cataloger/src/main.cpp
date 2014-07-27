////////////////////////////////Copyright © 2009////////////////////////////////
/*!
\file 		main.cpp
\author 	Mike Beach
\brief		
*/
////////////////////////////////////////////////////////////////////////////////

//Includes
#include <windows.h>
#include <assert.h>
#include <iostream>
#include <sys/stat.h>
#include <conio.h>
#include "mp3FileHandler.h"

//Fcn Declarations
string GetMusicCollectionPathForCurrentUser( void );
bool ForEachMp3File( const string& rootDir, bool(*pFcn)(const string&, bool, const string&) );
bool CatagorizeMp3IntoMusicCollection( const string& filename, bool fromCompAlbum = false, const string& compAlbumArtist = "Various Artists" );
bool IsACompilationAlbumDirectory( const string& dirName );
bool PromtForYesOrNoInput( const string& prompt );
bool DirectoryExists( const string& dirName );

//Global Constants
const char* DEFAULT_COMPILATION_ALBUM_ARTIST = "Various Artists";
const char* FOLDER_IMG_FILENAME = "folder.jpg";

//Global Variables
const string g_MusicCollectionPath = GetMusicCollectionPathForCurrentUser();


////////////////////////////////////////////////////////////////////////////////
/*!
\brief		
*/
////////////////////////////////////////////////////////////////////////////////
int main( int /*argc*/, char* /*argv[]*/ )
{
	int ret = 0;
	if( !DirectoryExists(g_MusicCollectionPath) )
	{
		printf( "The music collection directory \"%s\" doesn't exist, aborting...\n\n", g_MusicCollectionPath.c_str() );
		ret = -1;
	}
	else
	{
		ForEachMp3File( ".", CatagorizeMp3IntoMusicCollection );
	}

	printf( "Press any key to exit..." ); 
	_getch();

	return( ret );
}


////////////////////////////////////////////////////////////////////////////////
/*!
\brief		
*/
////////////////////////////////////////////////////////////////////////////////
string GetMusicCollectionPathForCurrentUser( void )
{
	string musicPathOut;
	const char* REG_KEY_FOR_SHELL_FOLDERS = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders";

	HKEY regKeyHandle;
	if( RegOpenKeyEx(HKEY_CURRENT_USER, REG_KEY_FOR_SHELL_FOLDERS, 0, KEY_QUERY_VALUE, &regKeyHandle) == ERROR_SUCCESS )
	{
		const char* REG_ENTRY_NAME_FOR_MUSIC_PATH = "My Music";
		const char* COLLECTION_SUBFOLDER = "\\My Collection\\";

		char musicPathBuff[ MAX_PATH ]	= { 0 };
		DWORD musicPathLen				= MAX_PATH;
		
		long result = RegQueryValueEx( regKeyHandle, REG_ENTRY_NAME_FOR_MUSIC_PATH, 0, 0, (LPBYTE)&musicPathBuff[0], &musicPathLen );
		assert( result == ERROR_SUCCESS ); // failed to query the entry we want, is the value name wrong?

		musicPathOut = musicPathBuff;
		musicPathOut += COLLECTION_SUBFOLDER;

		RegCloseKey( regKeyHandle );
	}
	else
	{
		assert( 0 ); // failed to open the key we want, is the sub-key wrong?
	}

	const char ENV_VAR_DELIM = '%';
	  // resolve environment variables
	int offIndx = 0, envVarIndx = string::npos;
	while( (envVarIndx = musicPathOut.find_first_of(ENV_VAR_DELIM, offIndx)) != string::npos )
	{
		int envVarBegin = envVarIndx + 1;
		int envVarEnd	= musicPathOut.find_first_of( ENV_VAR_DELIM, envVarBegin );
		assert( envVarEnd != string::npos ); // no ending delim?

		size_t	envVarSize = 0;
		char	envVarBuff[ MAX_PATH ] = {0};
		getenv_s( &envVarSize, envVarBuff, MAX_PATH,  musicPathOut.substr(envVarBegin, envVarEnd - envVarBegin).c_str() );
		assert( envVarSize > 0 ); // couldn't find the environment variable?

		musicPathOut = musicPathOut.substr( 0, envVarIndx ) + envVarBuff + musicPathOut.substr( envVarEnd+1 ); 
	}

	return( musicPathOut );
}


////////////////////////////////////////////////////////////////////////////////
/*!
\brief	
\return		True if the root directory is empty (can contain hidden system 
			files still), false if not.
*/
////////////////////////////////////////////////////////////////////////////////
bool ForEachMp3File( const string& rootDir, bool(*pFcn)(const string&, bool, const string&) )
{
	bool dirIsEmpty = true;

	const char* WILDCARD_FILE_SEARCH_STR = "\\*";	
	string searchStr = rootDir;
	searchStr.append( WILDCARD_FILE_SEARCH_STR );

	bool	isCompAlbumDir	= IsACompilationAlbumDirectory( rootDir );
	string	compAlbumArtist = DEFAULT_COMPILATION_ALBUM_ARTIST;
	  // if this is a compilation album directory and we want the album to be filed under something other than "Various Artists"
	if( isCompAlbumDir && !PromtForYesOrNoInput("Do you want to file the compilation album \"" + rootDir + "\" as a \"" + compAlbumArtist + "\" album?") )
	{
		std::cout << "Then please specify what \"artist\" you wish to file this album under: ";
		std::getline( std::cin, compAlbumArtist );
	}

	  // 
	  // recursively go through directories searching out mp3 files
	
	WIN32_FIND_DATA foundFileData;	
	HANDLE hFoundFile = FindFirstFile( searchStr.c_str(), &foundFileData );
	if( hFoundFile != INVALID_HANDLE_VALUE )
	{
		do
		{
			const char* CURR_DIRECTORY_FILENAME = ".";
			const char* PARENT_DIRECTORY_FILENAME = "..";

			  // if this is a directory  
			if( foundFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				  // if this directory isn't the current directory or the parent directory
				if( strcmp(foundFileData.cFileName, CURR_DIRECTORY_FILENAME) && strcmp(foundFileData.cFileName, PARENT_DIRECTORY_FILENAME) )
				{
					string foundDirPath = rootDir;
					foundDirPath.append( "\\" );
					foundDirPath.append( foundFileData.cFileName );

					if( ForEachMp3File(foundDirPath, pFcn) )
					{
						string rmDirCmd = "rmdir /s /q \"" + foundDirPath + "\"";
						system( rmDirCmd.c_str() );
					}
					else
					{
						dirIsEmpty = false;
					}
				}
				continue;
			}

			string foundFilePath = rootDir;
			foundFilePath.append( "\\" );
			foundFilePath.append( foundFileData.cFileName );

			if( Mp3FileParser::IsMp3Filename(foundFilePath) )
			{
				dirIsEmpty &= pFcn( foundFilePath, isCompAlbumDir, compAlbumArtist );
			}
			else // if this is some other file other than an mp3
			{
				DWORD fileAttrib = GetFileAttributes( foundFilePath.c_str() );
				  // if this file is not a hidden system file (like desktop.ini, folder.jpg, etc.), then this directory isn't empty
				dirIsEmpty &= ( fileAttrib != INVALID_FILE_ATTRIBUTES && (fileAttrib & FILE_ATTRIBUTE_HIDDEN) && (fileAttrib & FILE_ATTRIBUTE_SYSTEM) );
			}

		} while( FindNextFile(hFoundFile, &foundFileData) );

		FindClose( hFoundFile );
		assert( GetLastError() == ERROR_NO_MORE_FILES ); // make sure that the reason that FindNextFile() failed is that there are no more files to process 
	}

	return( dirIsEmpty );
}


////////////////////////////////////////////////////////////////////////////////
/*!
\brief		
*/
////////////////////////////////////////////////////////////////////////////////
bool CatagorizeMp3IntoMusicCollection( const string& filename, bool fromCompAlbum, const string& compAlbumArtist )
{
	static const Mp3FileParser fileParser;
	string newFilePath = g_MusicCollectionPath;

	  // if we're able to construct a proper filename for output
	if( fileParser.AppendResolvedOutputFilename(filename, newFilePath, fromCompAlbum, compAlbumArtist) )
	{
		string parentDirStr = newFilePath.substr( 0, newFilePath.find_last_of('\\') );
		  // make sure that the directory we're moving the mp3 into has been created 
		if( !DirectoryExists(parentDirStr.c_str()) )
		{
			CreateDirectory( parentDirStr.c_str(), NULL );
			
			const char* FOLDER_IMG_COPY_PATH = "cover.jpg";

			string curDirImgPath = filename.substr( 0, filename.find_last_of('\\') ) + "\\" + FOLDER_IMG_FILENAME;
			string dirImgCpyPath = parentDirStr + "\\" + FOLDER_IMG_COPY_PATH;
			string newDirImgPath = parentDirStr + "\\" + FOLDER_IMG_FILENAME;

			DWORD curFileAttrib = GetFileAttributes( curDirImgPath.c_str() );
			if( curFileAttrib != INVALID_FILE_ATTRIBUTES )
			{
				  // make a copy of the image file named "cover.jpg" (to keep the system from fucking with our images)
				CopyFile( curDirImgPath.c_str(), dirImgCpyPath.c_str(), true );
				SetFileAttributes( dirImgCpyPath.c_str(), curFileAttrib & ~(FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM) );	
			}
			else
			{
				printf( "Unable to copy \"%s\".\n\n", curDirImgPath.c_str() );
			}
			
			curFileAttrib = GetFileAttributes( dirImgCpyPath.c_str() );
			if( curFileAttrib != INVALID_FILE_ATTRIBUTES )
			{
				  // now make a copy of the system file to be displayed with the folder
				CopyFile( dirImgCpyPath.c_str(), newDirImgPath.c_str(), true );
				SetFileAttributes( newDirImgPath.c_str(), curFileAttrib | (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM) );
			}
		}

		  // copy the file using the new filename 
		if( CopyFile(filename.c_str(), newFilePath.c_str(), true) )
		{
			  // if the copy was successful, delete the old file.. we don't need it anymore
			remove( filename.c_str() );
			return( true );
		} 
	}

	  // if we've gotten this far, then we weren't able to move the mp3 file

	printf( "Unable to copy \"%s\" to \"%s\"\n\n", filename.c_str(), newFilePath.c_str() );
	return( false );
}


////////////////////////////////////////////////////////////////////////////////
/*!
\brief		
*/
////////////////////////////////////////////////////////////////////////////////
bool IsACompilationAlbumDirectory( const string& albumFilePath )
{
	size_t dirDelimIndx = albumFilePath.find_last_of('\\');
	  // here we assume that mp3 files are within an "..\album artist\album name" directory structure
	dirDelimIndx = albumFilePath.rfind( '\\', dirDelimIndx-1 );

	string compAlbumArtistDir = "\\" + string( DEFAULT_COMPILATION_ALBUM_ARTIST ) + "\\";
	if( dirDelimIndx != string::npos )
	{
		if( albumFilePath.find(compAlbumArtistDir, dirDelimIndx) == dirDelimIndx )
		{
			return( true );
		}
		
		string sndTrackAlbumArtistDir = "\\Soundtrack\\";
		string unknownAlbumArtistDir = "\\Unknown Artist\\";
		if( albumFilePath.find(sndTrackAlbumArtistDir, dirDelimIndx) == dirDelimIndx || albumFilePath.find(unknownAlbumArtistDir, dirDelimIndx) == dirDelimIndx )
		{
			if( PromtForYesOrNoInput("Is \"" + albumFilePath + "\" a compilation album?") )
			{
				return( true );
			}
		}
	}

	return( false );
}


////////////////////////////////////////////////////////////////////////////////
/*!
\brief		
*/
////////////////////////////////////////////////////////////////////////////////
bool PromtForYesOrNoInput( const string& prompt )
{
	const unsigned MAX_INPUT_LEN = 5;
	char inputStr[ MAX_INPUT_LEN ] = { 0 };

	for(;;) // while(1)
	{
		printf( "%s (y/n): ", prompt.c_str() );

		scanf_s( "%4s", inputStr, MAX_INPUT_LEN );
		fflush( stdin );

		  // make sure that the we're comparing lower case strings
		for( unsigned charIndx = 0; charIndx < MAX_INPUT_LEN-1; ++charIndx )
		{
			inputStr[ charIndx ] = (char)tolower( inputStr[charIndx] );
		}
		  
		if( !strcmp(inputStr, "yes") || !strcmp(inputStr, "y") ) // if the user answered yes
		{
			return( true );
		}
		else if( !strcmp(inputStr, "no") || !strcmp(inputStr, "n") ) // else if the user answered no
		{
			return( false );
		}

		printf( "Invalid input.\n", prompt.c_str() );
	}
}


////////////////////////////////////////////////////////////////////////////////
/*!
\brief		
*/
////////////////////////////////////////////////////////////////////////////////
bool DirectoryExists( const string& dirName )
{
	if( dirName.empty() )
	{
		return( false );
	}

	struct stat st;
	if( dirName[ dirName.size()-1 ] == '\\' )
	{
		return( stat(dirName.substr(0,dirName.size()-1).c_str(), &st) == 0 );
	}

	return( stat(dirName.c_str(), &st) == 0 );
}