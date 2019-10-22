@echo off
For /R %%G in (*.mp4) do (
mkdir "%%~dpG\gif"
call :to_gif 480 -1 "%%G" "%%~dpnG.gif.png" "%%~dpGgif\%%~nG.gif"
)
goto :eof

:to_gif
"D:\Program Files\FFmpeg\ffmpeg.exe" -y -i "%~3" -vf "null,pad=max(iw\,ih*(16/9)):ow/(16/9):(ow-iw)/2:(oh-ih)/2 ,scale=%~1:%~2:flags=lanczos,palettegen=stats_mode=full" -r 30 "%~4"
"D:\Program Files\FFmpeg\ffmpeg.exe" -y -i "%~3" -i "%~4" -lavfi "null,pad=max(iw\,ih*(16/9)):ow/(16/9):(ow-iw)/2:(oh-ih)/2 ,scale=%~1:%~2:flags=lanczos     [x]; [x][1:v] paletteuse=dither=bayer:bayer_scale=3" -r 30 "%~5"
DEL "%~4"
set size=0
set maxbytesize=10000000
call :getsize "%~5"
if %size% lss %maxbytesize% (
    echo File is less than %maxbytesize% bytes
) else (
    echo File is greater than or equal %maxbytesize% bytes
	call :to_gif %~1-10 %~2 "%~3" "%~4" "%~5"
)
goto :eof

:getsize
set size=%~z1
goto :eof