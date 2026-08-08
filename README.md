# Domaine Church Presenter

Full-screen live output for worship services — hymns, Bibles and announcements in one lightweight Windows program.

## Website

The landing page is in the `website/` folder and is published at GitHub Pages.

## Features

- Live full-screen output window + separate controller window
- Per-slide backgrounds and image-only slides
- Per-slide audio (MP3, MCI)
- Text color and shadow, lower-third mode, margins
- Loop playback, go-to-slide, alert messages
- Built-in Bible downloader (many translations, both testaments)
- Bundled public-domain hymn books
- Multi-language slide libraries (.txt files)

## Build

Requires MinGW GCC and a Windows SDK (WinInet, COMCTL32, WIC via ole32/oleaut32, WinMM):

```
gcc -Wall -O2 main.c -o "bin\Debug\church presentation.exe" ^
    -mwindows -lgdi32 -luser32 -lwininet -lcomctl32 -lole32 -loleaut32 -lwinmm
```

## Slide format

Slides are plain `.txt` files in a `slides` folder, one file per set, in any language:

```
[TITLE Welcome]
[BACKGROUND church.jpg]
[SLIDE 1]
Welcome to Domaine Church
Please turn off your phones
[SLIDE 2]
[COLOR 255,255,0]
[SHADOW 0,0,0]
[MARGIN 5]
Praise & Worship
[SLIDE 3]
[IMAGE photo.jpg]
[SLIDE 4]
[LOWERTHIRD]
Offering time
[SLIDE 5]
[AUDIO song.mp3]
```

## License

Free and open source.
