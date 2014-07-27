using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions; // for Regex
using System.Runtime.InteropServices; // for DllImport
using System.Diagnostics; // for Debug.Assert

namespace mp3_name_gen
{
    class CmdlineUtil
    {
        private static readonly string	CMDLINE_SWITCH_DELIMS	= @"(-|\\|/)";		// - or \ or / (using regex... http://www.mikesdotnetting.com/Article/46/CSharp-Regular-Expressions-Cheat-Sheet)
        private static readonly string	HELP_COMMANDS			= @"(\?|help|h)";	// ? or 'help' or h (also using regex)
        private static List<CmdlineArg> g_CmdlineArgList		= new List<CmdlineArg>();
        public delegate bool			CmdlineArgCb( string switchParam );

        /// <summary>
        /// 
        /// </summary>
        public static bool AddCmdArg( string cmdArg, CmdlineArgCb argCb, string paramDesc, string cmdDesc )
        {
            if( g_CmdlineArgList.Exists(arg => (arg.ArgumentStr == cmdArg)) )
            {
                Util.WriteWarningMsg( "NameGen.AddCmdArg - '{0}' already exists as a command argument.", cmdArg );
                return( false );
            }
            else if( Regex.IsMatch(cmdArg, HELP_COMMANDS) )
            {
                Util.WriteWarningMsg( "NameGen.AddCmdArg - '{0}' is reserved for listing help.", cmdArg );
                return( false );
            }

            g_CmdlineArgList.Add( new CmdlineArg(cmdArg, argCb, paramDesc, cmdDesc) );
            return( true );
        }

        /// <summary>
        /// 
        /// </summary>
        public static bool AddCmdArg( string cmdArg, CmdlineArgCb argCb, string cmdDesc )
        {
            return( AddCmdArg(cmdArg, argCb, "", cmdDesc) );
        }

        /// <summary>
        /// 
        /// </summary>
        public static bool ParseCmdline( string cmdlineArgStr )
        {
            if( Regex.IsMatch(cmdlineArgStr, "( |^)"+CMDLINE_SWITCH_DELIMS+HELP_COMMANDS+"( |$)") )
            {
                PrintHelp();
                return( false );
            }

            cmdlineArgStr = Regex.Replace( cmdlineArgStr, @"^\s+", "" ); // remove all preceeding whitespace
            if( cmdlineArgStr.Length <= 0 )
            {
                return( true ); // there was nothing to process... SUCCESS!
            }

            string[] args = SplitCmdlineIntoArgs(cmdlineArgStr);

            for( uint argIndx = 0; argIndx < args.Length; ++argIndx )
            {
                string argStr = args[ argIndx ];
                if( IsCmdSwitch(argStr) )
                {
                    string switchParam = (argIndx < args.Length-1) ? args[argIndx+1] : "";
                    if( IsCmdSwitch(switchParam) ) // if the next arg is another switch, then this must not need a param?
                    {
                        switchParam = "";
                    }
                    else
                    {
                        ++argIndx;
                    }

                    Match argDelim = Regex.Match( argStr, "^" + CMDLINE_SWITCH_DELIMS );
                    if( !HandleCmdSwitch(argStr.Substring(argDelim.Length), switchParam) )
                    {
                        // the param was invalid, or the arg doesn't exist
                    }

                    continue;
                }
                else if( !HandleCmdSwitch("", argStr) )
                {
                    Util.WriteErrorMsg( "Invalid cmd str '{0}', aborting...", argStr );
                    return( false );
                }
            }

            return( true );
        }

        /// <summary>
        /// 
        /// </summary>
        [DllImport("shell32.dll", SetLastError = true)]
        static extern IntPtr CommandLineToArgvW( [MarshalAs(UnmanagedType.LPWStr)] string lpCmdLine, out int pNumArgs );
        private static string[] SplitCmdlineIntoArgs(string commandLine)
        {
            int argc;
            var argv = CommandLineToArgvW(commandLine, out argc);        
            if (argv == IntPtr.Zero)
                throw new System.ComponentModel.Win32Exception();
            try
            {
                var args = new string[argc];
                for (var i = 0; i < args.Length; i++)
                {
                    var p = Marshal.ReadIntPtr(argv, i * IntPtr.Size);
                    args[i] = Marshal.PtrToStringUni(p);
                }

                return args;
            }
            finally
            {
                Marshal.FreeHGlobal(argv);
            }
        }

        /// <summary>
        /// 
        /// </summary>
        private static bool IsCmdSwitch( string arg )
        {
            return( Regex.IsMatch(arg, "^" + CMDLINE_SWITCH_DELIMS + @"\S") );
        }

        /// <summary>
        /// 
        /// </summary>
        private class CmdlineArg : Util.Pair<string,CmdlineArgCb> 
        { 
            public CmdlineArg(string cmdArg, CmdlineArgCb argAction, string paramDesc, string cmdDesc) : base(cmdArg, argAction) 
            {
                m_CmdDesc = cmdDesc;
                m_ParamDesc = paramDesc;
            }

            public string		ArgumentStr	{ get{return(First);} }
            public CmdlineArgCb CmdCallback	{ get{return(Second);} }
            public string		CmdDesc		{ get{return(m_CmdDesc);} }
            public string		ParamDesc	{ get{return(m_ParamDesc);} }
	
            private string		m_CmdDesc;	
            private string		m_ParamDesc;
        };

        /// <summary>
        /// 
        /// </summary>
        private static bool HandleCmdSwitch( string switchStr, string switchParam )
        {
            foreach( CmdlineArg arg in g_CmdlineArgList )
            {
                if( arg.ArgumentStr == switchStr )
                {
                    if( !arg.CmdCallback(switchParam) )
                    {
                        Util.WriteErrorMsg( "CmdlineUtil.HandleCmdSwitch - Invalid paramter '{0}' for the argument '{1}'.", switchParam, switchStr );
                        return( false );
                    }

                    return( true );
                }
            }

            return( false );
        }

        /// <summary>
        /// 
        /// </summary>
        private static void PrintHelp( )
        {
            int maxArgLen		= g_CmdlineArgList.Max( arg => arg.ArgumentStr.Length ) + 1; // +1 for spacing
            int maxParamDesc	= g_CmdlineArgList.Max( arg => arg.ParamDesc.Length )   + 3; // +3 for spacing and enclosing bracket chars

            string CMD_DELIM = "\\";
            Debug.Assert( Regex.IsMatch(CMD_DELIM, CMDLINE_SWITCH_DELIMS) );
            maxArgLen += CMD_DELIM.Length;

            string outputFmt = "{0,-" + maxArgLen.ToString() + "}{1,-" + maxParamDesc.ToString() + "}{2}";
			
            foreach( CmdlineArg arg in g_CmdlineArgList )
            {
                Debug.Assert( !Regex.IsMatch(arg.ArgumentStr,"\n") && !Regex.IsMatch(arg.ParamDesc,"\n") && !Regex.IsMatch(arg.CmdDesc,"^\n") );

                string[] cmdDescLines = Regex.Split( arg.CmdDesc, "\n" );
                Debug.Assert( cmdDescLines.Length >= 1 );

                string paramDesc = (arg.ParamDesc.Length > 0) ? "["+arg.ParamDesc+"]" : "";
                Console.WriteLine( string.Format(outputFmt, CMD_DELIM + arg.ArgumentStr, paramDesc, cmdDescLines[0]) );

                for( int extraLineIndx = 1; extraLineIndx < cmdDescLines.Length; ++extraLineIndx )
                {
                    string outNewlineFmt = "{0," + (maxArgLen + maxParamDesc + cmdDescLines[extraLineIndx].Length).ToString() + "}";
                    Console.WriteLine( string.Format(outNewlineFmt, cmdDescLines[extraLineIndx]) );
                }
            }
        }
    }
}
