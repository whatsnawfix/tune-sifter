
<#
.PARAMETER srcFmt
Optional param, which specifies the source file(s)'s name format (this acts as a fallback, incase 
the id3 tag doesn't contain all the information we need).

Use the following character sequences to denote a id3 data field in the filename:
  @a - Artist Name
  @d - Album Title 
  @n - Track Number
  @y - Release Year
  @t - Song Title 
  
.PARAMETER help
Prints out documentation describing how to use this script in detail.
#>

param(	[string] $srcPath 		= "",
		[string] $srcFmt 		= "",
		[string] $dstRootDir 	= "",
		[string] $dstFmt 		= "@a (@y) - @d\@a - @d - @n - @t",
		[switch] $noRecur		= [switch]::notpresent,
		[switch] $noImgCpy		= [switch]::notpresent,
		[switch] $help 			= [switch]::notpresent )
		
if( $help )
{
	Get-Help $MyInvocation.MyCommand.Definition -det
	return
}

if( $srcFmt -and !(Test-Path "$srcFmt" -isValid) )
{
	Write-Warning "The srcFmt filename '$srcFmt' is an invalid filename format; disregarding..."
	$srcFmt = ""
}

Write-Host "Resolving source path..."
if( $srcPath )
{
	if( !(Test-Path "$srcPath") )
	{
		Write-Error "The target source path '$srcPath' is invalid; aborting..."
		return
	}
	
	$srcPath = Convert-Path $srcPath
}
else
{
	$srcPath = (Get-Location).Path
}

Write-Host "Resolving destination path..."
if( !$dstRootDir )
{
	$COLLECTION_SUB_FOLDER = "\My Collection"

	$dstRootDir = (gp "HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders").PSObject.Members["My Music"].Value
	$dstRootDir += $COLLECTION_SUB_FOLDER
}

Write-Host "Compiling list of files to sort..."

$VALID_MP3_FILE_EXTENSION = ".mp3"
if( Test-Path $srcPath -pathType leaf )
{
	if( [System.IO.Path]::GetExtension($srcPath) -ne $VALID_MP3_FILE_EXTENSION )
	{
		Write-Error "The target source file '$srcPath' is not a valid mp3 file; aborting..."
		return
	}
	
	$filesToSort = $srcPath
}
else
{
	$gciExpr = "Get-ChildItem ""$srcPath"""
	if( !$noRecur )
	{
		$gciExpr += " -recurse"
	}

	$filesToSort = Invoke-Expression $gciExpr | where { [System.IO.Path]::GetExtension($_.Name) -eq $VALID_MP3_FILE_EXTENSION } | foreach { $_.FullName }
}

$coppiedImgs = @()

# image copy function
function CopyAlbumImg { param( [string]$srcImgPath, [string]$destImgDir ) 

	$coverImgPath = $destImgDir + "\cover.jpg"
	if( !(Test-Path "$coverImgPath") )
	{
		Copy-Item $srcImgPath $coverImgPath
		$coverImgFile = Get-Item -force $coverImgPath
		$coverImgFile.attributes = 'Normal'
	}
		
	$folderImgPath = $destImgDir + "\folder.jpg"
	Copy-Item -force $coverImgPath $folderImgPath
	[System.IO.File]::SetAttributes( $folderImgPath, [System.IO.FileAttributes]::Hidden )
	
	$script:coppiedImgs += $destImgDir
}

# prompt for image copy function
function PromptForAlbumImg { param( [string]$srcImgPath, [string]$destImgDir, [REF]$coppiedImgLog  ) 
	$promptTitle 	= "Album Cover?"
	$promptMsg   	= "Is '" + $srcImgPath + "' the album's cover image?"
	$promptYes 	 	= New-Object System.Management.Automation.Host.ChoiceDescription "&Yes", "Copies the specified image as the album's cover."
	$promptNo 	 	= New-Object System.Management.Automation.Host.ChoiceDescription "&No",  "Skips this image, and continues prompting through others."
	$promptOptions 	= [System.Management.Automation.Host.ChoiceDescription[]]( $promptYes, $promptNo )
	$promptResult 	= $host.ui.PromptForChoice($promptTitle, $promptMsg, $promptOptions, 1) 

	switch( $promptResult )
	{
		0 { CopyAlbumImg -srcImgPath $srcImgPath -destImgDir $destImgDir -coppiedImgLog $coppiedImgLog; return $true }
	}

	return $false	
}

# start constructing a call to the name gen xex
if( !$dstFmt.EndsWith($VALID_MP3_FILE_EXTENSION) )
{
	$dstFmt += $VALID_MP3_FILE_EXTENSION
}

$nameGenCmd = "-o ""$dstFmt"" "
if( $srcFmt )
{
	$nameGenCmd += "-f ""$srcFmt"" "
}

# words we want moved from the filename's start (for alphabetizing purposes, like "The Dogs" => "Dogs, The")
$nameGenCmd += " -w The "

# set it so that it'll apply and save any changes that're made to the id3
#$nameGenCmd += " -i "

# fields we want to be prompted to change (like we want to be able to set the artist for "Various Artist" albums)
$nameGenCmd += "-q ""@a,Unknown Artist"" " 
$nameGenCmd += "-q ""@a,Soundtrack"" " 
$nameGenCmd += "-q ""@d,Soundtrack"" " 
$nameGenCmd += "-q ""@a,Various"" " 
$nameGenCmd += "-q ""@a,Various Artists"" " 

# words we don't want capitalized
$nameGenCmd += " -l the ";
$nameGenCmd += " -l a ";
$nameGenCmd += " -l an ";
$nameGenCmd += " -l for ";
$nameGenCmd += " -l and ";
$nameGenCmd += " -l but ";
$nameGenCmd += " -l or ";
$nameGenCmd += " -l yet ";
$nameGenCmd += " -l so ";
$nameGenCmd += " -l to ";

[System.Reflection.Assembly]::LoadFrom( (Get-Location).Path + "\mp3_name_gen.dll" ) | out-null

$filesToSort | foreach { 
		#$_
		#$nameGenCmd
		$destFile = [mp3_name_gen.Interface]::Generate( $_, $nameGenCmd )
		# $env:larmoo
		# iex "$nameGenCmd ""$_"""
		# #echo "$nameGenCmd ""$_"""
		# $env:larmoo
		# #echo poo
		# $destFile = $dstRootDir + "\" + $destFile
		#$destFile
		$destDir = Split-Path $destFile
		[IO.Directory]::CreateDirectory( "$destDir" ) | out-null
		#$destFile
		#$destFile
		return
		
		
		
		if( !$noImgCpy )
		{					
			# copy "cover.jpg" if one exists for this album
			Get-ChildItem -force (Split-Path $_) | where { $_.Name -eq "cover.jpg" -and !($coppiedImgs -contains $destDir) } | foreach { 
				CopyAlbumImg $_.FullName $destDir
			}
			# copy "folder.jpg" if "cover.jpg" didn't exist
			Get-ChildItem -force (Split-Path $_) | where { $_.Name -eq "folder.jpg" -and !($coppiedImgs -contains $destDir) } | foreach { 
				CopyAlbumImg $_.FullName $destDir
			}
			
			# if "folder.jpg" and "cover.jpg" didn't exist... try to find any images and ask about them
			$otherImgList = Get-ChildItem -force -recurse (Split-Path $_) | where { $_.Extension -eq ".jpg" -and !($coppiedImgs -contains $destDir) } | Sort-Object FullName 
			for( $i = 0; $i -le ($otherImgList.Length - 1); ++$i ) { 
				if( (PromptForAlbumImg $otherImgList[$i].FullName $destDir) )
				{
					break 
				}
			}			
		}
	}