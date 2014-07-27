using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
//using System.Management.Automation;

namespace mp3_name_gen
{
    class Util
    {
        /// <summary>
        /// 
        /// </summary>
        //static void Main( string[] args )
        //{
        //    String[] arguments = Environment.GetCommandLineArgs();

        //    for( uint argIndx = 0; argIndx < args.Length; ++argIndx )
        //    {
        //        string argStr = args[ argIndx ];
        //        if( IsCmdSwitch(argStr) )
        //        {
        //            string switchParam = (argIndx < args.Length-1) ? args[argIndx+1] : null;
        //            if( HandleCmdSwitch(argStr, switchParam) )
        //            {
        //                ++argIndx;
        //            }

        //            if( g_HelpListed )
        //                return;

        //            continue;
        //        }
        //        else if( !NameGen.SetSrcFilePath(argStr) )
        //        {
        //            WriteErrorMsg( "Invalid cmd str '{0}', aborting...", argStr );
        //            return;
        //        }
        //    }

        //    if( g_OutPowershellVarName != "" )
        //    {
        //        //Console.WriteLine( g_OutPowershellVarName );

        //        var poshScript = "$global:larmoo = \"i hate u\"; $global:larmoo;";
        //        poshScript.Run();
        //        var poshScript2 = "$global:larmoo";
        //        poshScript2.Run();

        //        //System.Environment.SetEnvironmentVariable( "larmoo", "I HATE YOU", EnvironmentVariableTarget.User );
        //    }
        //    else
        //    {
        //        Console.WriteLine( NameGen.Generate() );
        //    }
        //}

        //private static string g_OutPowershellVarName = "";

        /// <summary>
        /// 
        /// </summary>
        public static void WriteErrorMsg( string format, params object[] args )
        {
            WriteColoredMsg( format, ConsoleColor.Black, ConsoleColor.Red, args );
        }

        /// <summary>
        /// 
        /// </summary>
        public static void WriteWarningMsg( string format, params object[] args )
        {
            WriteColoredMsg( format, ConsoleColor.Black, ConsoleColor.Yellow, args );
        }

        /// <summary>
        /// 
        /// </summary>
        public static bool PromptForYesNoInput( string promptMsg, bool defaultAnswer )
        {
            for(;;) // while 1
            {
                Console.WriteLine( promptMsg );

                if( defaultAnswer )
                {
                    Console.ForegroundColor = ConsoleColor.Yellow;
                }
                Console.Write( "[Y] " );
                Console.ResetColor();
                Console.Write( "Yes " );

                if( !defaultAnswer )
                {
                    Console.ForegroundColor = ConsoleColor.Yellow;
                }
                Console.Write( "[N] " );
                Console.ResetColor();
                Console.Write( "No (default is \"{0}\"): ", defaultAnswer ? 'Y' : 'N' );

                string input = Console.ReadLine();
                input = input.ToLower();

                if( input.Equals("y") || input.Equals("yes") )
                    return( true );
                else if( input.Equals("n") || input.Equals("no") )
                    return( false );
                else if( input.Length == 0 )
                    return( defaultAnswer );

                WriteErrorMsg( "'{0}' is not a valid answer, please try again.", input );
            }
        }

        /// <summary>
        /// 
        /// </summary>
        public class Pair<T, U> 
        {
            public Pair() {}

            public Pair( T first, U second ) 
            {
                this.First  = first;
                this.Second = second;
            }

            public T First { get; set; }
            public U Second { get; set; }
        };

        /// <summary>
        /// 
        /// </summary>
        private static void WriteColoredMsg( string format, ConsoleColor bgColor, ConsoleColor fgColor, params object[] args )
        {
            Console.BackgroundColor = bgColor;
            Console.ForegroundColor = fgColor;
            Console.WriteLine( format, args );
            Console.ResetColor();
        }

        //private static bool g_HelpListed = false;
        private static void WriteOutHelp( )
        {
            Console.WriteLine( "\\? \t\t\tList help (also works as \\h and \\help)." );
            Console.WriteLine( "\\i \t\t\tSave any id3 tag changes that are made." );


            Console.WriteLine( "\\f " );
                Console.WriteLine( "\t\t\t  (use the same special char sequences as \\o)." );
            Console.WriteLine( "\\n " );
            Console.WriteLine( "\\q " );
            Console.WriteLine( "\\v [Out PS Var Name] \tPowershell script variable that the generated name should go into." );

            //g_HelpListed = true;			
        }
    }
}
