attrib -S -H album*.jpg /S
attrib -S -H folder.jpg /S
attrib -H  zuneArt*.jpg /S

for /r %%i in (cove?.jpg) do (

	copy "%%i" "%%~dpifolder.jpg"

	for /f %%j in ('dir "%%~dpialbum*.jpg
	for /f %%k in ('dir "%%~dpizuneArt*.jpg
)

attrib +S +H album*.jpg /S
attrib +S +H folder.jpg /S
attrib +H  zuneArt*.jpg /S