using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using HundredMilesSoftware.UltraID3Lib;
using System.IO;
using System.Text.RegularExpressions;
using System.Diagnostics;
using System.Globalization;

namespace mp3_name_gen
{
    class NameGen
    {
        private static readonly string	ARTIST_SPEC_SEQ 		= "@a";
        private static readonly string	ALBUM_SPEC_SEQ			= "@d";
        private static readonly string	YEAR_SPEC_SEQ			= "@y";
        private static readonly string	TRACK_NUM_SPEC_SEQ		= "@n";
        private static readonly string	TITLE_SPEC_SEQ			= "@t";
        private static readonly string	MP3_FILE_EXTENSION		= ".mp3";

        private static string			g_FilePath;
        private static string			g_OutputStr				= "@a (@y) - @d\\@a - @d - @n - @t";
        private static int				g_TrackNumMinLen		= 2;
        private static List<string>		g_PrefixesToPostfix		= new List<string>();
        private static string			g_SrcFilenameFormat;
        private static List<string>		g_QuestionableNames		= new List<string>();
        private static bool				g_SaveId3				= false;
        private static bool				g_ApplyCapitalization	= true;
        private static List<string>		g_FixedLowerCaseWords	= new List<string>();
		
        /// <summary>
        /// 
        /// </summary>
        public static bool SetSrcFileNameFormat( string srcFilenameFmt )
        {
            g_SrcFilenameFormat = srcFilenameFmt;
            return( true );
        }

        /// <summary>
        /// 
        /// </summary>
        public static bool EnableSavingOfId3Changes( string unused )
        {
            //g_SaveId3 = true;
            return( true );
        }

        /// <summary>
        /// 
        /// </summary>
        public static bool DisableAutoCapitalization( string unused )
        {
            g_ApplyCapitalization = false;
            return( true );
        }
		
        /// <summary>
        /// 
        /// </summary>
        public static bool SetMinTrackNumLen( string minLenStr )
        {
            return( int.TryParse(minLenStr, out g_TrackNumMinLen) );
        }

        /// <summary>
        /// 
        /// </summary>
        public static bool SetOutputFormatting( string format )
        {
            if( format.Length == 0 )
            {
                Util.WriteErrorMsg( "Can't generate an empty name." );
                return( false );
            }

            g_OutputStr = format;
            return( true );
        }

        /// <summary>
        /// 
        /// </summary>
        public static bool SetSrcFilePath( string filePath )
        {
			//if( g_FilePath != null )
			//{
			//    Util.WriteWarningMsg( "Attempting to overwrite already set file path '{0}' with '{1}'.", g_FilePath, filePath );
			//}

            if( File.Exists(filePath) )
            {
                g_FilePath = filePath;
                return( true );
            }

            Util.WriteErrorMsg( "{0} is not a valid file path.", filePath );
            return( false );
        }

        /// <summary>
        /// 
        /// </summary>
        public static bool AddDisallowedPrefix( string badPrefix )
        {
            if( badPrefix == null || badPrefix.Length == 0 )
            {
                return( false );
            }

            if( !badPrefix.EndsWith(" ") )
            {
                badPrefix += ' ';
            }

            g_PrefixesToPostfix.Add( badPrefix );
            return( true );
        }

        /// <summary>
        /// 
        /// </summary>
        public static bool AddPromptableName( string questionableStr )
        {
            if( questionableStr == null || questionableStr.Length == 0 )
            {
                return( false );
            }

            if( !Regex.IsMatch(questionableStr, @"^@.,") )
            {
                return( false );
            }

            g_QuestionableNames.Add( questionableStr );
            return( true );
        }

        /// <summary>
        /// 
        /// </summary>
        public static bool AddLowerCaseWord( string lowerCaseWord )
        {
            if( lowerCaseWord == null || lowerCaseWord.Length == 0 )
            {
                return( false );
            }

            g_FixedLowerCaseWords.Add( lowerCaseWord.Trim() );
            return( true );
        }


        /// <summary>
        /// 
        /// </summary>
        private static string RetrieveTagInfoFromFilename( string id3FieldSeq )
        {
            if( g_SrcFilenameFormat == null || g_SrcFilenameFormat.Length == 0 )
            {
                return( "" );
            }

            Debug.Assert( id3FieldSeq.StartsWith("@") && id3FieldSeq.Length == 2 );

            string filename = Path.GetFileName( g_FilePath );
            string[] tokenizedStr = Regex.Split( g_SrcFilenameFormat, @"(@.)" );

            string regExStr = "";
            string foundStr = "";

            for( int tokenIndx = 0; tokenIndx < tokenizedStr.Length; ++tokenIndx )
            {
                if( tokenIndx % 2 != 0 )
                {
                    Debug.Assert( tokenizedStr[tokenIndx].StartsWith("@") && tokenizedStr[tokenIndx].Length == 2 );

                    if( tokenizedStr[tokenIndx].Equals(id3FieldSeq) )
                    {
                        Match exprMatch = Regex.Match( filename, regExStr );
                        Debug.Assert( exprMatch.Index == 0 );

                        foundStr = filename.Substring( exprMatch.Length );
                        regExStr  = "";
                    }
                    else
                    {
                        regExStr += @".+";
                    }
                }
                else
                {
                    regExStr += tokenizedStr[ tokenIndx ];
                }
            }

            if( foundStr.Length > 0 )
            {
                Match endExprMatch = Regex.Match( foundStr, regExStr );
                foundStr = foundStr.Substring( 0, endExprMatch.Index );
            }

            return( foundStr );
        }

        /// <summary>
        /// 
        /// </summary>
        private delegate bool ValidSequenceStringCb( string strToTest );
        private static string ScreenSpecialSequenceStr( string specialSeqChar, string replacementStr, string filename, ValidSequenceStringCb validateCb )
        {
            foreach( string promptableStr in (from fishyStr in g_QuestionableNames where fishyStr.StartsWith(specialSeqChar+",", true, null) select fishyStr.Substring(specialSeqChar.Length+1)) )
            {
                if( promptableStr == replacementStr )
                {
                    Util.WriteWarningMsg( "Questionable '{0}' sequence for '{1}'.", specialSeqChar, filename );

                    if( !Util.PromptForYesNoInput("Do you want the '" + specialSeqChar + "' sequence char to be filled in as \"" + replacementStr +  "\"?", false) )
                    {
                        Console.Write( "Then please specify what you would rather it be instead: " );
                        replacementStr = Console.ReadLine();

                        while( !validateCb(replacementStr) )
                        {
                            Util.WriteErrorMsg( "'{0}' is not a valid sequence for '{1}'", replacementStr, specialSeqChar );
                            Console.Write( "Please re-enter what you'd rather it be: " );
                            replacementStr = Console.ReadLine();
                        }
                    }

                    break;
                }
            }

            return( replacementStr );
        }

        /// <summary>
        /// 
        /// </summary>
        private static bool ValidStr( string strToTest )
        {
            if( strToTest == null || strToTest.Length == 0 )
            {
                return( false );
            }

            return( true );
        }

        /// <summary>
        /// 
        /// </summary>
        private static bool ValidNumericalStr( string strToTest )
        {
            if( !ValidStr(strToTest) )
            {
                return( false );
            }

            bool strIsNumeric = true;
            try
            {
                short.Parse( strToTest );
            }
            catch
            {
                strIsNumeric = false;
            }

            return( strIsNumeric ); 
        }

        /// <summary>
        /// 
        /// </summary>
        private static bool ValidYearStr( string strToTest )
        {
            if( !ValidNumericalStr(strToTest) )
            {
                return( false );
            }

            return( strToTest.Length == 4 ); 
        }

        private static void ChangeEntireDirId3Tags( string dirPath, string specialSeqChar, string replacementStr, bool recursePrompt )
        {
            string[] fileList = Directory.GetFiles( dirPath, "*" + MP3_FILE_EXTENSION, SearchOption.TopDirectoryOnly );

            if( recursePrompt && Directory.GetDirectories(dirPath).Length > 0 && Directory.GetFiles(dirPath, "*" + MP3_FILE_EXTENSION, SearchOption.AllDirectories).Length > fileList.Length && 
                Util.PromptForYesNoInput("Would you like to make this change recursively, to all subdirectories as well?", false) )
            {
                fileList = Directory.GetFiles( dirPath, "*" + MP3_FILE_EXTENSION, SearchOption.AllDirectories );
            }			

            UltraID3 id3Tag = new UltraID3();
            foreach( string filename in fileList )
            {
                try
                {
                    id3Tag.Read( filename );
                }
                catch( UltraID3Exception ex )
                {
                    Util.WriteErrorMsg( ex.Message );
                    continue;
                }

                if( specialSeqChar == ARTIST_SPEC_SEQ )
                {
                    id3Tag.Artist = replacementStr;
                }
                else if( specialSeqChar == ALBUM_SPEC_SEQ )
                {
                    id3Tag.Album = replacementStr;
                }
                else if( specialSeqChar == YEAR_SPEC_SEQ )
                {
                    id3Tag.SetYear( replacementStr );
                }
                else if( specialSeqChar == TRACK_NUM_SPEC_SEQ )
                {
                    id3Tag.SetTrackNum( replacementStr );
                }
                else if( specialSeqChar == TITLE_SPEC_SEQ )
                {
                    id3Tag.Title = replacementStr;
                }

                id3Tag.Write();
            }
        }
		
        /// <summary>
        /// 
        /// </summary>
        static bool PromptToPropegateChange( string filePath, string specialSeqChar, string replacementStr )
        {
            if( !g_SaveId3 )
            {
                return( false );
            }

            Debug.Assert( Path.GetExtension(filePath).ToLower() == MP3_FILE_EXTENSION, "NameGen.PromptToPropegateChange - Is this not a " + MP3_FILE_EXTENSION + " file!?" );

            string fileDir = Path.GetDirectoryName( filePath );
            if( Directory.GetFiles(fileDir).Length > 1 )
            {
                if( Util.PromptForYesNoInput("Do you want to make this same change to all " + MP3_FILE_EXTENSION + " files within the same directory (" + fileDir + ")?", false) )
                {
                    ChangeEntireDirId3Tags( fileDir, specialSeqChar, replacementStr, true );
                    return( true );
                }
            }
            else
            {
                string[] subDirs = Directory.GetDirectories( fileDir, "*", SearchOption.AllDirectories );

                if( subDirs.Length > 0 && Directory.GetFiles(fileDir, "*" + MP3_FILE_EXTENSION, SearchOption.AllDirectories).Length > 1 && 
                    Util.PromptForYesNoInput("Do you want to make this same change to all sub-directories (to all " + MP3_FILE_EXTENSION + " files found under " + fileDir + ")?", false) )
                {
                    foreach( string dir in subDirs )
                    {
                        ChangeEntireDirId3Tags( dir, specialSeqChar, replacementStr, false );
                    }
                    return( true );
                }
            }

            return( false );
        }

        /// <summary>
        /// 
        /// </summary>
        private static string ApplyCapitalizationRules( string strToXform )
        {
            if( !g_ApplyCapitalization )
            {
                return( strToXform );
            }

            StringBuilder xformedStr = new StringBuilder( strToXform );
            int firstWordIndx = Regex.Match( strToXform, @"^\s*\S+" ).Length;
            int lastWordIndx  = Regex.Match( strToXform, @"\S+\s*$" ).Index;
			
            foreach( Match regexMatch in Regex.Matches(strToXform, @"[^\s-]+") )
            {
                int charIndx = regexMatch.Index;
                if( charIndx > firstWordIndx && charIndx < lastWordIndx && 
                    g_FixedLowerCaseWords.Exists(word => word.ToLower() == regexMatch.Value.ToLower()) )
                {
                    continue;	
                }
					
                xformedStr[ regexMatch.Index ] = char.ToUpper( xformedStr[regexMatch.Index] );
            }

            return( xformedStr.ToString() );
        }


        /// <summary>
        /// 
        /// </summary>
        private static void FixUpId3Tag( ref UltraID3 id3Tag )
        {
            ValidSequenceStringCb validStrCb	 = new ValidSequenceStringCb( ValidStr );
            ValidSequenceStringCb validNumStrCb	 = new ValidSequenceStringCb( ValidNumericalStr );
            ValidSequenceStringCb validYearStrCb = new ValidSequenceStringCb( ValidYearStr );

            if( id3Tag.Artist.Length == 0 )
            {
                id3Tag.Artist = RetrieveTagInfoFromFilename( ARTIST_SPEC_SEQ );
            }
            string newId3Str = ScreenSpecialSequenceStr( ARTIST_SPEC_SEQ, id3Tag.Artist, id3Tag.FileName, validStrCb );
            if( id3Tag.Artist != newId3Str )
            {
                id3Tag.Artist = newId3Str;
                PromptToPropegateChange( id3Tag.FileName, ARTIST_SPEC_SEQ, newId3Str );
            }
            id3Tag.Artist = ApplyCapitalizationRules( id3Tag.Artist );

            if( id3Tag.Album.Length == 0 )
            {
                id3Tag.Album = RetrieveTagInfoFromFilename( ALBUM_SPEC_SEQ );
            }
            newId3Str = ScreenSpecialSequenceStr( ALBUM_SPEC_SEQ, id3Tag.Album, id3Tag.FileName, validStrCb );
            if( id3Tag.Album != newId3Str )
            {
                id3Tag.Album = newId3Str;
                PromptToPropegateChange( id3Tag.FileName, ALBUM_SPEC_SEQ, newId3Str );
            }
            id3Tag.Album = ApplyCapitalizationRules( id3Tag.Album );

            short newYear;
            if( id3Tag.Year == null )
            {
                if( short.TryParse(RetrieveTagInfoFromFilename(YEAR_SPEC_SEQ), out newYear) )
                {
                    id3Tag.Year = newYear;
                }
            }
            newId3Str = ScreenSpecialSequenceStr( YEAR_SPEC_SEQ, id3Tag.Year.ToString(), id3Tag.FileName, validYearStrCb );
            if( short.TryParse(newId3Str, out newYear) )
            {
                if( id3Tag.Year != newYear )
                {
                    id3Tag.Year = newYear;
                    PromptToPropegateChange( id3Tag.FileName, YEAR_SPEC_SEQ, newId3Str );
                }
            }

            short newTrackNum;
            if( id3Tag.TrackNum == null )
            {
                if( short.TryParse(RetrieveTagInfoFromFilename(TRACK_NUM_SPEC_SEQ), out newTrackNum) )
                {
                    id3Tag.TrackNum = newTrackNum;
                }
            }
            newId3Str = ScreenSpecialSequenceStr( TRACK_NUM_SPEC_SEQ, id3Tag.TrackNum.ToString(), id3Tag.FileName, validNumStrCb );
            if( short.TryParse(newId3Str, out newTrackNum) )
            {
                if( id3Tag.TrackNum != newTrackNum )
                {
                    id3Tag.TrackNum = newTrackNum;
                    PromptToPropegateChange( id3Tag.FileName, TRACK_NUM_SPEC_SEQ, newId3Str );
                }
            }

            if( id3Tag.Title.Length == 0 )
            {
                id3Tag.Title = RetrieveTagInfoFromFilename( TITLE_SPEC_SEQ );
            }
            newId3Str = ScreenSpecialSequenceStr( TITLE_SPEC_SEQ, id3Tag.Title, id3Tag.FileName, validStrCb );
            if( id3Tag.Title != newId3Str )
            {
                id3Tag.Title = newId3Str;
                PromptToPropegateChange( id3Tag.FileName, TITLE_SPEC_SEQ, newId3Str );
            }
            id3Tag.Title = ApplyCapitalizationRules( id3Tag.Title );
        }

        /// <summary>
        /// 
        /// </summary>
        private static string FillInFormatStr( UltraID3 id3Tag, string formatStr )
        {
            string filledInStr = formatStr;

            filledInStr = filledInStr.Replace( ARTIST_SPEC_SEQ, id3Tag.Artist );
            filledInStr  = filledInStr.Replace( ALBUM_SPEC_SEQ, id3Tag.Album );
            filledInStr = filledInStr.Replace( YEAR_SPEC_SEQ, id3Tag.Year.ToString() );

            // properly format the track number string (with leading zeroes)
            int		trackNum		= (int)id3Tag.TrackNum;
            int		numLeadingZeros = Math.Max( g_TrackNumMinLen - (int)trackNum.ToString().Length, 0 ) + 1;
            string	trackNumStr		= trackNum.ToString("D" + numLeadingZeros.ToString() ); 
            filledInStr				= filledInStr.Replace( TRACK_NUM_SPEC_SEQ, trackNumStr );

            filledInStr = filledInStr.Replace( TITLE_SPEC_SEQ, id3Tag.Title );

            return( filledInStr );
        }

        /// <summary>
        /// 
        /// </summary>
        public static string Generate( string srcFilePath )
        {
            if( g_OutputStr == null || g_OutputStr.Length == 0 )
            {
                Util.WriteErrorMsg( "Can't generate an empty string." );
                return( "" );
            }

            if( !SetSrcFilePath(srcFilePath) )
            {
                Util.WriteErrorMsg( "Failed to set source file path." );
                return( Path.GetFileName(srcFilePath) );
            }

            UltraID3 id3Tag = new UltraID3();

            try
            {
                id3Tag.Read( g_FilePath );
            }
            catch( UltraID3Exception ex )
            {
                Util.WriteErrorMsg( ex.Message );
                return( "" );
            }

			Console.WriteLine( id3Tag.Artist );

            AddPromptableName( ARTIST_SPEC_SEQ+',' );
            AddPromptableName( ALBUM_SPEC_SEQ+',' );
            AddPromptableName( TRACK_NUM_SPEC_SEQ+',' );
            AddPromptableName( YEAR_SPEC_SEQ+',' );
            AddPromptableName( TITLE_SPEC_SEQ+',' );
            FixUpId3Tag( ref id3Tag );

            Regex regExParsser = new Regex( @"(@.)" );
            string[] tokenizedStr = regExParsser.Split( g_OutputStr );

            string generatedName = tokenizedStr[0];
            // check and move any disallowed prefixes (like change "The Foo" to "Foo, The")
            if( tokenizedStr.Length > 1 )
            {
                generatedName += FillInFormatStr( id3Tag, tokenizedStr[1] );

                if( tokenizedStr[0].Length == 0 )
                {
					StringBuilder fixedUpStr = new StringBuilder(generatedName);

					bool postFixed = false;
					foreach( string prefixStr in g_PrefixesToPostfix )
					{
					    if( generatedName.StartsWith(prefixStr, true, null) )
					    {
					        fixedUpStr = fixedUpStr.Remove( 0, prefixStr.Length );

					        if( !postFixed )
					        {
					            fixedUpStr.Append( ", " );
					        }

					        fixedUpStr.Append( prefixStr );
					        postFixed = true;
					    }
					}
						
					if( postFixed )
					{	
					    // make sure the first and last words are capitalized 
					    if( g_ApplyCapitalization )
					    {
					        int firstLetIndx = Regex.Match(fixedUpStr.ToString(), @"^\s*").Length;
					        fixedUpStr[ firstLetIndx ] = Char.ToUpper( fixedUpStr[firstLetIndx] );

					        int endLetIndx = Regex.Match(fixedUpStr.ToString(), @"\S+\s*$").Index;
					        fixedUpStr[ endLetIndx ] = Char.ToUpper( fixedUpStr[endLetIndx] );
					    }

					    generatedName = fixedUpStr.ToString();

					    if( generatedName.EndsWith(" ") && tokenizedStr[2].StartsWith(" ") )
					    {
					        generatedName = generatedName.Remove( generatedName.Length-1 );
					    }
					}
                }

                generatedName += tokenizedStr[2];
            }

            for( uint strIndx = 3; strIndx < tokenizedStr.Length; strIndx += 2 )
            {
                generatedName += FillInFormatStr( id3Tag, tokenizedStr[strIndx] ) + tokenizedStr[strIndx+1];
            }

            if( g_SaveId3 )
            {
                id3Tag.Write();
            }
 
			Console.WriteLine( generatedName );
            return( generatedName );
        }
    }
}
