using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO; // for Path

namespace mp3_name_gen
{
	[Serializable]
	public class Interface
	{
		public static string Generate( string srcFilePath )
		{
			return( Generate(srcFilePath, "") ); 
		}

		public static string Generate( string srcFilePath, string cmdlineArgs )
		{
			AddDefaultCmdSet();
			if( !CmdlineUtil.ParseCmdline(cmdlineArgs) )
			{
			    return( Path.GetFileName(srcFilePath) );
			}

			return( NameGen.Generate(srcFilePath) );
		}

		static bool g_CmdlineAlreadyInitd = false;
		private static void AddDefaultCmdSet( )
		{			
			if( g_CmdlineAlreadyInitd == true )
			{
			    return;
			}

			CmdlineUtil.AddCmdArg( "o", new CmdlineUtil.CmdlineArgCb(NameGen.SetOutputFormatting),	"Output Format Str",
			    "Format of the desired output string." +
			    "\n  @a Artist Name	" +
			    "\n  @d Album Title	" +
			    "\n  @n Track Number" +
			    "\n  @y Release Year" +
			    "\n  @t Song Title"   );

			CmdlineUtil.AddCmdArg( "f", new CmdlineUtil.CmdlineArgCb(NameGen.SetSrcFileNameFormat), "Src Filename Fmt",
			    "RegEx format of the src filename (as an id3 back fall)." +
			    "\n  (use the same special char sequences as \\o)." );

			CmdlineUtil.AddCmdArg( "w", new CmdlineUtil.CmdlineArgCb(NameGen.AddDisallowedPrefix),	"Disallowed Prefix",
			    "A string that will be removed from the output's start." +
			    "\nThe string will be post-fixed instead, like so:" + 
			    "\n  \"The Dancing Turtle\" => \"Dancing Turtle, The\"" );

			CmdlineUtil.AddCmdArg( "n", new CmdlineUtil.CmdlineArgCb(NameGen.SetMinTrackNumLen),	"Min Track Num Len",
			    "The min length of the track num (will pad with zeroes)." );
			CmdlineUtil.AddCmdArg( "q", new CmdlineUtil.CmdlineArgCb(NameGen.AddPromptableName),	"charSeq,FishyStr",
			    "A str to be propted to change for the char sequence." );
			CmdlineUtil.AddCmdArg( "i", new CmdlineUtil.CmdlineArgCb(NameGen.EnableSavingOfId3Changes), 
			    "Save any id3 tag changes that are made." );
			CmdlineUtil.AddCmdArg( "l", new CmdlineUtil.CmdlineArgCb(NameGen.AddLowerCaseWord),		"LowerCase Word",
			    "A word that'll be lower-cased (if capitalizing)." );
			CmdlineUtil.AddCmdArg( "c", new CmdlineUtil.CmdlineArgCb(NameGen.DisableAutoCapitalization), 
			    "Disables Auto-Capitalization (as if they're titles)." );

			g_CmdlineAlreadyInitd = true;
		}
	}
}
