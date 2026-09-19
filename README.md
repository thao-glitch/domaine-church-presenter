# Domaine Church

The modern home for the whole church family — members, services, media, chat and online sessions. Plus a stage presenter for the big screen.

Live at: **https://thao-glitch.github.io/domaine-church-presenter/**

## What's inside

**Web app** (`webapp/`, React + Vite + TypeScript, powered by Supabase):

- **Role-based app** — every member logs in and the app shows what their role can manage:
  Bishop → Pastor → Elder → Deacon/Minister/Worship Leader → Usher → Member, plus Media (stage control).
- **Members directory** — everyone, sorted by role rank. Leaders add/edit/remove members.
- **Services & Events** — weekly schedule and one-off services/events (worldwide service-type list).
- **Media uploads** — every member can upload songs, notes, photos, recordings; leaders curate.
- **Chat** — general channels, your ministry's channel, and private messages, live.
- **Online sessions** — live video meetings built on **LiveKit** (Jitsi-free, professional video).
- **Stage (Presenter)** — an embedded, modern take on the classic desktop presenter: build presentations in the browser and push them to the church TV screen in full-screen mode.
- **Dashboard** — today's services and events for the whole congregation.

**Windows desktop presenter** (`main.c`) — the original lightweight stage tool, still shipped at `bin/Debug/`.

## Setup (one time, by the church leadership)

1. Create a free project at **supabase.com** → Project Settings → API. Copy the project URL and the anon (public) key.
2. In the Supabase SQL editor run **`webapp/supabase/schema.sql`** (creates: profiles, members, services, events, channels, messages, files, sessions, slide_sets, slides, stage + security rules + storage bucket + seed data).
3. Create accounts under **Authentication → Users**, then add each person to the **profiles** table with their email and role.
4. For video meetings: create a free project at **livekit.io/cloud**, then deploy the token function (`webapp/supabase/functions/livekit-token`) with the LiveKit API key/secret. See **`webapp/supabase/LIVEKIT.md`**.
5. Open the app → the **Setup** screen asks for the Supabase URL/key and LiveKit details (they are stored in your own browser).

## Build the web app

```
cd webapp
npm install
npm run build     # outputs to ../docs, then copies the supabase docs alongside
npm run dev       # local dev server
```

## Build the desktop presenter

Requires MinGW GCC and the Windows SDK links:

```
gcc -Wall -O2 main.c -o "bin\Debug\church presentation.exe" ^
    -mwindows -lgdi32 -luser32 -lwininet -lcomctl32 -lole32 -loleaut32 -lwinmm
```

## Slide format (desktop app)

Slides are plain `.txt` files in a `slides` folder, one file per set:

```
[TITLE Welcome]
[BACKGROUND church.jpg]
[SLIDE 1]
Welcome to Domaine Church
[SLIDE 2]
[COLOR 255,255,0]
[SHADOW 0,0,0]
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