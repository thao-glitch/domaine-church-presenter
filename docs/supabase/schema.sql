-- ============================================================
-- Domaine Church: full Supabase schema (baseline)
-- ------------------------------------------------------------
-- Generated from supabase/migrations/*.sql (0001 -> 0003).
-- Fresh install: run this whole file once in the SQL editor.
-- ============================================================

-- ============================================================
-- Domaine Church â€” full Supabase schema (members app + stage)
-- ------------------------------------------------------------
-- Run the whole file in the Supabase SQL editor with the app
-- offline (or before users sign up). It is safe to re-run.
-- ============================================================

create extension if not exists pgcrypto;

-- ---------------------------------------------------------- profiles
create table if not exists public.profiles (
  id uuid primary key references auth.users (id) on delete cascade,
  email text not null unique,
  full_name text,
  role text not null default 'Member',
  created_at timestamptz not null default now()
);
alter table public.profiles enable row level security;
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.profiles'::regclass and polname = 'profiles_select') then
    create policy "profiles_select" on public.profiles for select using (true);
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.profiles'::regclass and polname = 'profiles_insert_own') then
    create policy "profiles_insert_own" on public.profiles for insert with check (auth.uid() = id);
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.profiles'::regclass and polname = 'profiles_update_own') then
    create policy "profiles_update_own" on public.profiles for update using (auth.uid() = id);
  end if;
end $$;

-- ---------------------------------------------------------- roles
-- Ranks matching the app's role list. These two functions are the
-- single place where "who may edit/schedule/present" is decided by
-- the database (server-enforced, not just hidden in the UI).
-- (Defined after public.profiles so the table exists at creation.)
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
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.members'::regclass and polname = 'members_select') then
    create policy "members_select" on public.members for select using (true);
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.members'::regclass and polname = 'members_write') then
    create policy "members_write" on public.members for all using (public.is_editor()) with check (public.is_editor());
  end if;
end $$;

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
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.services'::regclass and polname = 'services_select') then
    create policy "services_select" on public.services for select using (true);
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.services'::regclass and polname = 'services_write') then
    create policy "services_write" on public.services for all using (public.is_editor()) with check (public.is_editor());
  end if;
end $$;

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
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.events'::regclass and polname = 'events_select') then
    create policy "events_select" on public.events for select using (true);
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.events'::regclass and polname = 'events_write') then
    create policy "events_write" on public.events for all using (public.is_editor()) with check (public.is_editor());
  end if;
end $$;

-- ----------------------------------------------------- channels
create table if not exists public.channels (
  id uuid primary key default gen_random_uuid(),
  key text not null unique,
  name text not null,
  kind text not null default 'general',
  created_at timestamptz not null default now()
);
alter table public.channels enable row level security;
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.channels'::regclass and polname = 'channels_select') then
    create policy "channels_select" on public.channels for select using (auth.uid() is not null);
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.channels'::regclass and polname = 'channels_insert') then
    create policy "channels_insert" on public.channels for insert with check (auth.uid() is not null);
  end if;
end $$;

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
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.messages'::regclass and polname = 'messages_select') then
    create policy "messages_select" on public.messages for select using (auth.uid() is not null);
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.messages'::regclass and polname = 'messages_insert') then
    create policy "messages_insert" on public.messages for insert with check (auth.uid() is not null);
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.messages'::regclass and polname = 'messages_delete') then
    create policy "messages_delete" on public.messages for delete using (public.is_editor() or sender = auth.jwt() ->> 'email');
  end if;
end $$;
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
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.files'::regclass and polname = 'files_select') then
    create policy "files_select" on public.files for select using (auth.uid() is not null);
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.files'::regclass and polname = 'files_insert') then
    create policy "files_insert" on public.files for insert with check (auth.uid() is not null);
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.files'::regclass and polname = 'files_delete') then
    create policy "files_delete" on public.files for delete using (public.is_editor() or uploaded_by = auth.jwt() ->> 'email');
  end if;
end $$;

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
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.sessions'::regclass and polname = 'sessions_select') then
    create policy "sessions_select" on public.sessions for select using (auth.uid() is not null);
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.sessions'::regclass and polname = 'sessions_write') then
    create policy "sessions_write" on public.sessions for all using (public.is_editor()) with check (public.is_editor());
  end if;
end $$;

-- ------------------------------------------------ slide_sets
create table if not exists public.slide_sets (
  id uuid primary key default gen_random_uuid(),
  title text not null,
  owner text,
  created_at timestamptz not null default now()
);
alter table public.slide_sets enable row level security;
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.slide_sets'::regclass and polname = 'slide_sets_select') then
    create policy "slide_sets_select" on public.slide_sets for select using (auth.uid() is not null);
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.slide_sets'::regclass and polname = 'slide_sets_write') then
    create policy "slide_sets_write" on public.slide_sets for all using (public.is_presenter()) with check (public.is_presenter());
  end if;
end $$;

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
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.slides'::regclass and polname = 'slides_select') then
    create policy "slides_select" on public.slides for select using (auth.uid() is not null);
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.slides'::regclass and polname = 'slides_write') then
    create policy "slides_write" on public.slides for all using (public.is_presenter()) with check (public.is_presenter());
  end if;
end $$;
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
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.stage'::regclass and polname = 'stage_select') then
    create policy "stage_select" on public.stage for select using (auth.uid() is not null);
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.stage'::regclass and polname = 'stage_update') then
    create policy "stage_update" on public.stage for update using (public.is_presenter() or public.is_editor()) with check (public.is_presenter() or public.is_editor());
  end if;
end $$;

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

-- ============================================================
-- Domaine Church â€” multi-church upgrade
-- ------------------------------------------------------------
-- Adds the churches table, a church_id column on every data table,
-- and swaps all security rules to "your church only".
-- Existing rows are attached to a host church (Domaine Church).
-- Re-runnable: safe on an empty or partially-migrated database.
-- ============================================================

-- ----------------------------------------------------------- churches
create table if not exists public.churches (
  id uuid primary key default gen_random_uuid(),
  name text not null,
  slug text not null unique,
  city text,
  country text,
  description text,
  contact_email text,
  owner_email text,
  owner_name text,
  created_at timestamptz not null default now()
);
alter table public.churches enable row level security;

-- RLS: anyone logged-in may view/register a church; only its owner edits it.
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.churches'::regclass and polname = 'churches_select') then
    create policy "churches_select" on public.churches for select using (auth.uid() is not null);
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.churches'::regclass and polname = 'churches_insert') then
    create policy "churches_insert" on public.churches for insert with check (true);
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.churches'::regclass and polname = 'churches_update') then
    create policy "churches_update" on public.churches for update using (owner_email = auth.jwt() ->> 'email');
  end if;
end $$;

-- Host church that absorbs any existing data.
insert into public.churches (name, slug, city, country, description, owner_email)
values ('Domaine Church', 'domaine-church', null, null, 'Home church for existing data.', null)
on conflict (slug) do nothing;

-- ------------------------------------------------- church_id columns
do $$
declare v_church uuid;
begin
  select id into v_church from public.churches where slug = 'domaine-church' limit 1;
  if v_church is null then
    insert into public.churches (name, slug) values ('Domaine Church', 'domaine-church') on conflict (slug) do nothing;
    select id into v_church from public.churches where slug = 'domaine-church' limit 1;
  end if;

  alter table public.profiles    add column if not exists church_id uuid references public.churches (id) on delete set null;
  alter table public.members     add column if not exists church_id uuid references public.churches (id) on delete cascade;
  alter table public.services    add column if not exists church_id uuid references public.churches (id) on delete cascade;
  alter table public.events      add column if not exists church_id uuid references public.churches (id) on delete cascade;
  alter table public.channels    add column if not exists church_id uuid references public.churches (id) on delete cascade;
  alter table public.messages    add column if not exists church_id uuid references public.churches (id) on delete cascade;
  alter table public.files       add column if not exists church_id uuid references public.churches (id) on delete cascade;
  alter table public.sessions    add column if not exists church_id uuid references public.churches (id) on delete cascade;
  alter table public.slide_sets  add column if not exists church_id uuid references public.churches (id) on delete cascade;
  alter table public.stage       add column if not exists church_id uuid references public.churches (id) on delete cascade;

  update public.profiles   set church_id = v_church where church_id is null;
  update public.members    set church_id = v_church where church_id is null;
  update public.services   set church_id = v_church where church_id is null;
  update public.events     set church_id = v_church where church_id is null;
  update public.channels   set church_id = v_church where church_id is null;
  update public.messages   set church_id = v_church where church_id is null;
  update public.files      set church_id = v_church where church_id is null;
  update public.sessions   set church_id = v_church where church_id is null;
  update public.slide_sets set church_id = v_church where church_id is null;
  update public.stage      set church_id = v_church where church_id is null;
end $$;

-- Channels: each church owns its channel keys.
alter table public.channels drop constraint if exists channels_key_key;
alter table public.channels add constraint channels_church_key_key unique (church_id, key);

-- Stage: one row per church instead of a global single row.
alter table public.stage drop constraint if exists stage_pkey;
alter table public.stage add constraint stage_church_key_key unique (church_id);

create index if not exists messages_church_idx on public.messages (church_id, created_at);
create index if not exists members_church_idx on public.members (church_id);
create index if not exists sessions_church_idx on public.sessions (church_id, starts_at);
create index if not exists files_church_idx on public.files (church_id, created_at);

-- ------------------------------------------- church-scoped helpers
create or replace function public.my_church_id()
returns uuid language sql stable security definer set search_path = public as $$
  select church_id from public.profiles
  where email = auth.jwt() ->> 'email' and church_id is not null
  limit 1;
$$;

-- ----------------------------------------------- church-scoped RLS
-- Swap every policy to operate strictly inside the signed-in user's church.

do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.profiles'::regclass and polname = 'profiles_select') then
    create policy "profiles_select" on public.profiles for select using (church_id = public.my_church_id());
  else
    drop policy "profiles_select" on public.profiles;
    create policy "profiles_select" on public.profiles for select using (church_id = public.my_church_id());
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.profiles'::regclass and polname = 'profiles_insert_own') then
    create policy "profiles_insert_own" on public.profiles for insert with check (auth.uid() = id);
  else
    drop policy "profiles_insert_own" on public.profiles;
    create policy "profiles_insert_own" on public.profiles for insert with check (auth.uid() = id);
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.profiles'::regclass and polname = 'profiles_update_own') then
    create policy "profiles_update_own" on public.profiles for update using (auth.uid() = id) with check (auth.uid() = id);
  else
    drop policy "profiles_update_own" on public.profiles;
    create policy "profiles_update_own" on public.profiles for update using (auth.uid() = id) with check (auth.uid() = id);
  end if;
  -- church editors may assign roles to profiles inside their own church
  if not exists (select 1 from pg_policy where polrelid = 'public.profiles'::regclass and polname = 'profiles_role_update') then
    create policy "profiles_role_update" on public.profiles for update using (public.is_editor() and church_id = public.my_church_id()) with check (public.is_editor() and church_id = public.my_church_id());
  else
    drop policy "profiles_role_update" on public.profiles;
    create policy "profiles_role_update" on public.profiles for update using (public.is_editor() and church_id = public.my_church_id()) with check (public.is_editor() and church_id = public.my_church_id());
  end if;
end $$;

do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.members'::regclass and polname = 'members_select') then
    create policy "members_select" on public.members for select using (church_id = public.my_church_id());
  else
    drop policy "members_select" on public.members;
    create policy "members_select" on public.members for select using (church_id = public.my_church_id());
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.members'::regclass and polname = 'members_write') then
    create policy "members_write" on public.members for all using (public.is_editor() and church_id = public.my_church_id()) with check (public.is_editor() and church_id = public.my_church_id());
  else
    drop policy "members_write" on public.members;
    create policy "members_write" on public.members for all using (public.is_editor() and church_id = public.my_church_id()) with check (public.is_editor() and church_id = public.my_church_id());
  end if;
end $$;

do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.services'::regclass and polname = 'services_select') then
    create policy "services_select" on public.services for select using (church_id = public.my_church_id());
  else
    drop policy "services_select" on public.services;
    create policy "services_select" on public.services for select using (church_id = public.my_church_id());
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.services'::regclass and polname = 'services_write') then
    create policy "services_write" on public.services for all using (public.is_editor() and church_id = public.my_church_id()) with check (public.is_editor() and church_id = public.my_church_id());
  else
    drop policy "services_write" on public.services;
    create policy "services_write" on public.services for all using (public.is_editor() and church_id = public.my_church_id()) with check (public.is_editor() and church_id = public.my_church_id());
  end if;
end $$;

do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.events'::regclass and polname = 'events_select') then
    create policy "events_select" on public.events for select using (church_id = public.my_church_id());
  else
    drop policy "events_select" on public.events;
    create policy "events_select" on public.events for select using (church_id = public.my_church_id());
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.events'::regclass and polname = 'events_write') then
    create policy "events_write" on public.events for all using (public.is_editor() and church_id = public.my_church_id()) with check (public.is_editor() and church_id = public.my_church_id());
  else
    drop policy "events_write" on public.events;
    create policy "events_write" on public.events for all using (public.is_editor() and church_id = public.my_church_id()) with check (public.is_editor() and church_id = public.my_church_id());
  end if;
end $$;

do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.channels'::regclass and polname = 'channels_select') then
    create policy "channels_select" on public.channels for select using (church_id = public.my_church_id());
  else
    drop policy "channels_select" on public.channels;
    create policy "channels_select" on public.channels for select using (church_id = public.my_church_id());
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.channels'::regclass and polname = 'channels_insert') then
    create policy "channels_insert" on public.channels for insert with check (auth.uid() is not null and church_id = public.my_church_id());
  else
    drop policy "channels_insert" on public.channels;
    create policy "channels_insert" on public.channels for insert with check (auth.uid() is not null and church_id = public.my_church_id());
  end if;
end $$;

do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.messages'::regclass and polname = 'messages_select') then
    create policy "messages_select" on public.messages for select using (church_id = public.my_church_id());
  else
    drop policy "messages_select" on public.messages;
    create policy "messages_select" on public.messages for select using (church_id = public.my_church_id());
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.messages'::regclass and polname = 'messages_insert') then
    create policy "messages_insert" on public.messages for insert with check (auth.uid() is not null and church_id = public.my_church_id() and sender = auth.jwt() ->> 'email');
  else
    drop policy "messages_insert" on public.messages;
    create policy "messages_insert" on public.messages for insert with check (auth.uid() is not null and church_id = public.my_church_id() and sender = auth.jwt() ->> 'email');
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.messages'::regclass and polname = 'messages_delete') then
    create policy "messages_delete" on public.messages for delete using (public.is_editor() or sender = auth.jwt() ->> 'email');
  else
    drop policy "messages_delete" on public.messages;
    create policy "messages_delete" on public.messages for delete using (public.is_editor() or sender = auth.jwt() ->> 'email');
  end if;
end $$;

do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.files'::regclass and polname = 'files_select') then
    create policy "files_select" on public.files for select using (church_id = public.my_church_id());
  else
    drop policy "files_select" on public.files;
    create policy "files_select" on public.files for select using (church_id = public.my_church_id());
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.files'::regclass and polname = 'files_insert') then
    create policy "files_insert" on public.files for insert with check (auth.uid() is not null and church_id = public.my_church_id());
  else
    drop policy "files_insert" on public.files;
    create policy "files_insert" on public.files for insert with check (auth.uid() is not null and church_id = public.my_church_id());
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.files'::regclass and polname = 'files_delete') then
    create policy "files_delete" on public.files for delete using (public.is_editor() or uploaded_by = auth.jwt() ->> 'email');
  else
    drop policy "files_delete" on public.files;
    create policy "files_delete" on public.files for delete using (public.is_editor() or uploaded_by = auth.jwt() ->> 'email');
  end if;
end $$;

do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.sessions'::regclass and polname = 'sessions_select') then
    create policy "sessions_select" on public.sessions for select using (church_id = public.my_church_id());
  else
    drop policy "sessions_select" on public.sessions;
    create policy "sessions_select" on public.sessions for select using (church_id = public.my_church_id());
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.sessions'::regclass and polname = 'sessions_write') then
    create policy "sessions_write" on public.sessions for all using (public.is_editor() and church_id = public.my_church_id()) with check (public.is_editor() and church_id = public.my_church_id());
  else
    drop policy "sessions_write" on public.sessions;
    create policy "sessions_write" on public.sessions for all using (public.is_editor() and church_id = public.my_church_id()) with check (public.is_editor() and church_id = public.my_church_id());
  end if;
end $$;

do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.slide_sets'::regclass and polname = 'slide_sets_select') then
    create policy "slide_sets_select" on public.slide_sets for select using (church_id = public.my_church_id());
  else
    drop policy "slide_sets_select" on public.slide_sets;
    create policy "slide_sets_select" on public.slide_sets for select using (church_id = public.my_church_id());
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.slide_sets'::regclass and polname = 'slide_sets_write') then
    create policy "slide_sets_write" on public.slide_sets for all using (public.is_presenter() and church_id = public.my_church_id()) with check (public.is_presenter() and church_id = public.my_church_id());
  else
    drop policy "slide_sets_write" on public.slide_sets;
    create policy "slide_sets_write" on public.slide_sets for all using (public.is_presenter() and church_id = public.my_church_id()) with check (public.is_presenter() and church_id = public.my_church_id());
  end if;
end $$;

do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.slides'::regclass and polname = 'slides_select') then
    create policy "slides_select" on public.slides for select using (set_id in (select id from public.slide_sets where church_id = public.my_church_id()));
  else
    drop policy "slides_select" on public.slides;
    create policy "slides_select" on public.slides for select using (set_id in (select id from public.slide_sets where church_id = public.my_church_id()));
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.slides'::regclass and polname = 'slides_write') then
    create policy "slides_write" on public.slides for all using (public.is_presenter() and set_id in (select id from public.slide_sets where church_id = public.my_church_id())) with check (public.is_presenter() and set_id in (select id from public.slide_sets where church_id = public.my_church_id()));
  else
    drop policy "slides_write" on public.slides;
    create policy "slides_write" on public.slides for all using (public.is_presenter() and set_id in (select id from public.slide_sets where church_id = public.my_church_id())) with check (public.is_presenter() and set_id in (select id from public.slide_sets where church_id = public.my_church_id()));
  end if;
end $$;

do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.stage'::regclass and polname = 'stage_select') then
    create policy "stage_select" on public.stage for select using (church_id = public.my_church_id());
  else
    drop policy "stage_select" on public.stage;
    create policy "stage_select" on public.stage for select using (church_id = public.my_church_id());
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.stage'::regclass and polname = 'stage_update') then
    create policy "stage_update" on public.stage for update using ((public.is_presenter() or public.is_editor()) and church_id = public.my_church_id()) with check ((public.is_presenter() or public.is_editor()) and church_id = public.my_church_id());
  else
    drop policy "stage_update" on public.stage;
    create policy "stage_update" on public.stage for update using ((public.is_presenter() or public.is_editor()) and church_id = public.my_church_id()) with check ((public.is_presenter() or public.is_editor()) and church_id = public.my_church_id());
  end if;
end $$;

-- ============================================================
-- Admins: church admins per church + one overall platform admin
-- ------------------------------------------------------------
-- Safe to re-run.
-- ============================================================

-- ------------------------------------------- platform admins
create table if not exists public.platform_admin_emails (
  email text primary key,
  created_at timestamptz not null default now()
);
alter table public.platform_admin_emails enable row level security;
-- No RLS policies on purpose: only the SECURITY DEFINER helper reads it.

insert into public.platform_admin_emails (email)
  values ('johnkamonyegitau@gmail.com')
  on conflict (email) do nothing;

create or replace function public.is_platform_admin()
returns boolean language sql stable security definer set search_path = public as $$
  select exists (
    select 1 from public.platform_admin_emails a
    where lower(a.email) = lower(auth.jwt() ->> 'email')
  );
$$;

-- ------------------------------------------- church admins
-- Church Admin role + the top leadership ranks may manage users/roles.
create or replace function public.is_church_admin()
returns boolean language sql stable security definer set search_path = public as $$
  select public.is_platform_admin() or exists (
    select 1 from public.profiles p
    where p.email = auth.jwt() ->> 'email'
      and p.role in ('Church Admin','Bishop','Senior Pastor','Pastor','Assistant Pastor','Elder')
  );
$$;

-- Refresh edit/present sets so "Church Admin" has full powers.
create or replace function public.is_editor()
returns boolean language sql stable security definer set search_path = public as $$
  select public.is_platform_admin() or exists (
    select 1 from public.profiles p
    where p.email = auth.jwt() ->> 'email'
      and p.role in (
        'Church Admin','Bishop','Senior Pastor','Pastor','Assistant Pastor',
        'Elder','Deacon','Deaconess','Evangelist',
        'Minister','Worship Leader','Youth Leader'
      )
  );
$$;

create or replace function public.is_presenter()
returns boolean language sql stable security definer set search_path = public as $$
  select public.is_platform_admin() or exists (
    select 1 from public.profiles p
    where p.email = auth.jwt() ->> 'email'
      and p.role in ('Church Admin','Bishop','Senior Pastor','Pastor','Assistant Pastor','Elder','Media')
  );
$$;

-- ------------------------------------------- role management
-- Only church admins assign roles; everyone else is locked to Member.
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.profiles'::regclass and polname = 'profiles_role_update') then
    create policy "profiles_role_update" on public.profiles for update
      using (public.is_church_admin() and church_id = public.my_church_id())
      with check (public.is_church_admin() and church_id = public.my_church_id());
  else
    drop policy "profiles_role_update" on public.profiles;
    create policy "profiles_role_update" on public.profiles for update
      using (public.is_church_admin() and church_id = public.my_church_id())
      with check (public.is_church_admin() and church_id = public.my_church_id());
  end if;
end $$;

create or replace function public.guard_profile_role()
returns trigger language plpgsql security definer set search_path = public as $$
begin
  if not public.is_church_admin() then
    if tg_op = 'INSERT' then
      new.role := 'Member';
    elsif new.role is distinct from old.role then
      raise exception 'Only a church admin can change roles.';
    end if;
  end if;
  return new;
end $$;

drop trigger if exists profiles_guard_role on public.profiles;
create trigger profiles_guard_role
  before insert or update of role on public.profiles
  for each row execute function public.guard_profile_role();

-- Church admins / platform admin may remove linked accounts.
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.profiles'::regclass and polname = 'profiles_delete_admin') then
    create policy "profiles_delete_admin" on public.profiles for delete
      using (public.is_platform_admin() or (public.is_church_admin() and church_id = public.my_church_id()));
  else
    drop policy "profiles_delete_admin" on public.profiles;
    create policy "profiles_delete_admin" on public.profiles for delete
      using (public.is_platform_admin() or (public.is_church_admin() and church_id = public.my_church_id()));
  end if;
end $$;

-- ------------------------------------------- churches
-- Owner or platform admin may edit/delete a church.
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.churches'::regclass and polname = 'churches_admin') then
    create policy "churches_admin" on public.churches for all
      using (public.is_platform_admin() or owner_email = (auth.jwt() ->> 'email'))
      with check (public.is_platform_admin() or owner_email = (auth.jwt() ->> 'email'));
  else
    drop policy "churches_admin" on public.churches;
    create policy "churches_admin" on public.churches for all
      using (public.is_platform_admin() or owner_email = (auth.jwt() ->> 'email'))
      with check (public.is_platform_admin() or owner_email = (auth.jwt() ->> 'email'));
  end if;
end $$;

-- ------------------------------------------- platform admin: all data
-- Additive permissive policies so the overall admin can see and manage
-- every church's app end to end.
do $$
declare t text;
begin
  foreach t in array array['profiles','members','services','events','channels','messages','files','sessions','slide_sets','slides','stage','churches'] loop
    if not exists (
      select 1 from pg_policy
      where polrelid = ('public.' || t)::regclass
        and polname = t || '_platform_admin'
    ) then
      execute format(
        'create policy %I on public.%I for all using (public.is_platform_admin()) with check (public.is_platform_admin())',
        t || '_platform_admin', t
      );
    end if;
  end loop;
end $$;

-- ------------------------------------------- clean test accounts
-- Remove every registered account except the overall admin, so the
-- church registrations start from a clean slate.
delete from auth.users u
  where lower(coalesce(u.email, '')) <> 'johnkamonyegitau@gmail.com';
