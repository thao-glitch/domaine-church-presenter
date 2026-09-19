-- ============================================================
-- Domaine Church — full Supabase schema (members app + stage)
-- ------------------------------------------------------------
-- Run the whole file in the Supabase SQL editor with the app
-- offline (or before users sign up). It is safe to re-run.
--
-- Then, to let users log in, create accounts under
-- Authentication -> Users and add them to "profiles" with a role
-- (see ROLE_PERMISSIONS below).
-- ============================================================

create extension if not exists pgcrypto;

-- ---------------------------------------------------------- roles
-- Ranks matching the app's role list. These two functions are the
-- single place where "who may edit/schedule/present" is decided by
-- the database (server-enforced, not just hidden in the UI).
create or replace function public.is_editor()
returns boolean language sql stable security definer set search_path = public as $$
  select exists (
    select 1 from public.profiles p
    where p.email = auth.jwt() ->> 'email'
      and p.role in (
        'Bishop', 'Senior Pastor', 'Pastor', 'Assistant Pastor',
        'Elder', 'Deacon', 'Deaconess', 'Evangelist',
        'Minister', 'Worship Leader', 'Youth Leader'
      )
  );
$$;

create or replace function public.is_presenter()
returns boolean language sql stable security definer set search_path = public as $$
  select exists (
    select 1 from public.profiles p
    where p.email = auth.jwt() ->> 'email'
      and p.role in ('Bishop', 'Senior Pastor', 'Pastor', 'Assistant Pastor', 'Elder', 'Media')
  );
$$;

-- ---------------------------------------------------------- profiles
create table if not exists public.profiles (
  id uuid primary key references auth.users (id) on delete cascade,
  email text not null unique,
  full_name text,
  role text not null default 'Member',
  created_at timestamptz not null default now()
);
alter table public.profiles enable row level security;
drop policy if exists "profiles_select" on public.profiles;
create policy "profiles_select" on public.profiles for select using (true);
drop policy if exists "profiles_insert_own" on public.profiles;
create policy "profiles_insert_own" on public.profiles for insert with check (auth.uid() = id);
drop policy if exists "profiles_update_own" on public.profiles;
create policy "profiles_update_own" on public.profiles for update using (auth.uid() = id);

-- ---------------------------------------------------------- members
create table if not exists public.members (
  id uuid primary key default gen_random_uuid(),
  full_name text not null,
  phone text,
  email text,
  role text not null default 'Member',
  created_at timestamptz not null default now()
);
alter table public.members enable row level security;
drop policy if exists "members_select" on public.members;
create policy "members_select" on public.members for select using (true);
drop policy if exists "members_write" on public.members for all using (public.is_editor()) with check (public.is_editor());

-- ------------------------------------------------------ services
create table if not exists public.services (
  id uuid primary key default gen_random_uuid(),
  name text not null,
  type text,
  recurring boolean not null default false,
  weekday int,
  date date,
  start_time text,
  end_time text,
  location text,
  created_at timestamptz not null default now()
);
alter table public.services enable row level security;
drop policy if exists "services_select" on public.services;
create policy "services_select" on public.services for select using (true);
drop policy if exists "services_write" on public.services for all using (public.is_editor()) with check (public.is_editor());

-- ---------------------------------------------------------- events
create table if not exists public.events (
  id uuid primary key default gen_random_uuid(),
  title text not null,
  category text,
  date date not null,
  start_time text,
  end_time text,
  location text,
  description text,
  created_at timestamptz not null default now()
);
alter table public.events enable row level security;
drop policy if exists "events_select" on public.events;
create policy "events_select" on public.events for select using (true);
drop policy if exists "events_write" on public.events for all using (public.is_editor()) with check (public.is_editor());

-- ----------------------------------------------------- channels
create table if not exists public.channels (
  id uuid primary key default gen_random_uuid(),
  key text not null unique,
  name text not null,
  kind text not null default 'general',
  created_at timestamptz not null default now()
);
alter table public.channels enable row level security;
drop policy if exists "channels_select" on public.channels for select using (auth.uid() is not null);
drop policy if exists "channels_insert" on public.channels for insert with check (auth.uid() is not null);

-- ------------------------------------------------------ messages
create table if not exists public.messages (
  id uuid primary key default gen_random_uuid(),
  channel_key text not null,
  sender text not null,
  sender_name text,
  body text not null,
  created_at timestamptz not null default now()
);
alter table public.messages enable row level security;
drop policy if exists "messages_select" on public.messages for select using (auth.uid() is not null);
drop policy if exists "messages_insert" on public.messages for insert with check (auth.uid() is not null);
drop policy if exists "messages_delete" on public.messages for delete using (public.is_editor() or sender = auth.jwt() ->> 'email');
create index if not exists messages_channel_idx on public.messages (channel_key, created_at);

-- ----------------------------------------------------------- files
create table if not exists public.files (
  id uuid primary key default gen_random_uuid(),
  name text not null,
  path text not null,
  mime text,
  size bigint not null default 0,
  category text not null default 'Other',
  uploaded_by text not null,
  uploaded_by_name text,
  created_at timestamptz not null default now()
);
alter table public.files enable row level security;
drop policy if exists "files_select" on public.files for select using (auth.uid() is not null);
drop policy if exists "files_insert" on public.files for insert with check (auth.uid() is not null);
drop policy if exists "files_delete" on public.files
  for delete using (public.is_editor() or uploaded_by = auth.jwt() ->> 'email');

-- storage bucket for uploads (public read so files open for everyone)
insert into storage.buckets (id, name, public)
values ('uploads', 'uploads', true)
on conflict (id) do nothing;

-- ------------------------------------------------------- sessions
create table if not exists public.sessions (
  id uuid primary key default gen_random_uuid(),
  title text not null,
  host text,
  starts_at timestamptz not null,
  ends_at timestamptz,
  room text not null,
  description text,
  created_at timestamptz not null default now()
);
alter table public.sessions enable row level security;
drop policy if exists "sessions_select" on public.sessions for select using (auth.uid() is not null);
drop policy if exists "sessions_write" on public.sessions
  for all using (public.is_editor()) with check (public.is_editor());

-- ------------------------------------------------------ slide_sets
create table if not exists public.slide_sets (
  id uuid primary key default gen_random_uuid(),
  title text not null,
  owner text,
  created_at timestamptz not null default now()
);
alter table public.slide_sets enable row level security;
drop policy if exists "slide_sets_select" on public.slide_sets for select using (auth.uid() is not null);
drop policy if exists "slide_sets_write" on public.slide_sets
  for all using (public.is_presenter()) with check (public.is_presenter());

-- ----------------------------------------------------------- slides
create table if not exists public.slides (
  id uuid primary key default gen_random_uuid(),
  set_id uuid not null references public.slide_sets (id) on delete cascade,
  idx int not null default 0,
  text text not null default '',
  bg text not null default '#0b0f1a',
  created_at timestamptz not null default now()
);
alter table public.slides enable row level security;
drop policy if exists "slides_select" on public.slides for select using (auth.uid() is not null);
drop policy if exists "slides_write" on public.slides
  for all using (public.is_presenter()) with check (public.is_presenter());
create index if not exists slides_set_idx on public.slides (set_id, idx);

-- ------------------------------------------------------------ stage
create table if not exists public.stage (
  id int primary key,
  set_id uuid references public.slide_sets (id) on delete set null,
  slide_idx int not null default 0,
  updated_at timestamptz not null default now()
);
insert into public.stage (id, set_id, slide_idx) values (1, null, 0) on conflict (id) do nothing;
alter table public.stage enable row level security;
drop policy if exists "stage_select" on public.stage for select using (auth.uid() is not null);
drop policy if exists "stage_update" on public.stage
  for update using (public.is_presenter() or public.is_editor()) with check (public.is_presenter() or public.is_editor());

-- ------------------------------------------------ realtime sync
do $$
begin
  alter publication supabase_realtime add table public.messages, public.stage, public.files, public.sessions;
exception when duplicate_object then
  null;
end $$;

-- --------------------------------------------------- seed services
insert into public.services (name, type, recurring, weekday, start_time, end_time, location)
values
  ('Sunday Worship', 'Sunday Worship Service', true, 0, '10:00', '12:30', 'Main Church'),
  ('Sunday Prayer', 'Prayer Meeting', true, 0, '08:30', '09:15', 'Prayer Hall'),
  ('Bible Study', 'Bible Study', true, 2, '18:30', '20:00', 'Fellowship Hall'),
  ('Wednesday Prayer', 'Prayer Meeting', true, 3, '18:00', '19:30', 'Prayer Hall'),
  ('Youth Service', 'Youth Service', true, 5, '17:00', '19:00', 'Youth Hall'),
  ('Choir Practice', 'Choir Practice', true, 4, '17:30', '19:00', 'Choir Room')
on conflict do nothing;

-- channels used by the chat sidebar
insert into public.channels (key, name, kind) values
  ('general', 'General', 'general'),
  ('prayer', 'Prayer Pointers', 'general'),
  ('announcements', 'Announcements', 'general')
on conflict (key) do nothing;