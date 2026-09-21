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


-- ============================================================
-- Church life: content engine, groups, planning, attendance,
-- prayer, RSVP, giving, recordings.  Church-scoped, safe to re-run.
-- ============================================================

-- ----------------------------------------------------------- groups
create table if not exists public.groups (
  id uuid primary key default gen_random_uuid(),
  church_id uuid not null references public.churches (id) on delete cascade,
  name text not null,
  kind text not null default 'ministry',
  description text not null default '',
  leader_email text,
  meeting_day int,
  meeting_time text,
  location text,
  created_at timestamptz not null default now()
);
alter table public.groups enable row level security;

create table if not exists public.group_members (
  id uuid primary key default gen_random_uuid(),
  church_id uuid not null references public.churches (id) on delete cascade,
  group_id uuid not null references public.groups (id) on delete cascade,
  profile_id uuid references public.profiles (id) on delete cascade,
  name text,
  member_role text not null default 'member',
  created_at timestamptz not null default now(),
  unique (group_id, profile_id)
);
alter table public.group_members enable row level security;

-- ------------------------------------------------------ content engine
create table if not exists public.content_items (
  id uuid primary key default gen_random_uuid(),
  church_id uuid references public.churches (id) on delete cascade,
  scope text not null default 'church' check (scope in ('global','church','group')),
  kind text not null default 'announcement',
  title text not null,
  body text not null default '',
  link_url text,
  audience text not null default 'all',
  group_id uuid references public.groups (id) on delete set null,
  state text not null default 'draft' check (state in ('draft','scheduled','published','archived')),
  publish_at timestamptz,
  expires_at timestamptz,
  channels text[] not null default '{app}',
  created_by text,
  created_at timestamptz not null default now(),
  updated_at timestamptz not null default now()
);
alter table public.content_items enable row level security;

-- ---------------------------------------------------- service planning
create table if not exists public.service_plans (
  id uuid primary key default gen_random_uuid(),
  church_id uuid not null references public.churches (id) on delete cascade,
  title text not null,
  plan_date date not null default current_date,
  service_id uuid references public.services (id) on delete set null,
  theme text,
  notes text,
  created_at timestamptz not null default now()
);
alter table public.service_plans enable row level security;

create table if not exists public.service_plan_items (
  id uuid primary key default gen_random_uuid(),
  church_id uuid not null references public.churches (id) on delete cascade,
  plan_id uuid not null references public.service_plans (id) on delete cascade,
  position int not null default 0,
  item_type text not null default 'custom',
  title text not null,
  person text,
  duration_min int,
  notes text,
  created_at timestamptz not null default now()
);
alter table public.service_plan_items enable row level security;

create table if not exists public.volunteer_slots (
  id uuid primary key default gen_random_uuid(),
  church_id uuid not null references public.churches (id) on delete cascade,
  plan_id uuid references public.service_plans (id) on delete set null,
  role text not null,
  title text,
  service_date date,
  needed int not null default 1,
  created_at timestamptz not null default now()
);
alter table public.volunteer_slots enable row level security;

create table if not exists public.volunteer_assignments (
  id uuid primary key default gen_random_uuid(),
  church_id uuid not null references public.churches (id) on delete cascade,
  slot_id uuid not null references public.volunteer_slots (id) on delete cascade,
  profile_id uuid references public.profiles (id) on delete set null,
  name text,
  status text not null default 'invited',
  created_at timestamptz not null default now()
);
alter table public.volunteer_assignments enable row level security;

-- ------------------------------------------------ attendance & follow-up
create table if not exists public.attendance (
  id uuid primary key default gen_random_uuid(),
  church_id uuid not null references public.churches (id) on delete cascade,
  service_date date not null default current_date,
  service_id uuid references public.services (id) on delete set null,
  person_name text not null,
  profile_id uuid references public.profiles (id) on delete set null,
  status text not null default 'present',
  checked_in_at timestamptz not null default now()
);
alter table public.attendance enable row level security;

create table if not exists public.follow_ups (
  id uuid primary key default gen_random_uuid(),
  church_id uuid not null references public.churches (id) on delete cascade,
  person_name text not null,
  phone text,
  email text,
  is_first_timer boolean not null default true,
  stage text not null default 'new',
  assigned_to text,
  notes text not null default '',
  created_at timestamptz not null default now(),
  updated_at timestamptz not null default now()
);
alter table public.follow_ups enable row level security;

-- ----------------------------------------------------------- prayer wall
create table if not exists public.prayers (
  id uuid primary key default gen_random_uuid(),
  church_id uuid not null references public.churches (id) on delete cascade,
  author_email text not null,
  author_name text,
  body text not null,
  is_private boolean not null default false,
  status text not null default 'open',
  group_id uuid references public.groups (id) on delete set null,
  pray_count int not null default 0,
  created_at timestamptz not null default now()
);
alter table public.prayers enable row level security;

-- ------------------------------------------------------------- rsvps
create table if not exists public.rsvps (
  id uuid primary key default gen_random_uuid(),
  church_id uuid not null references public.churches (id) on delete cascade,
  event_id uuid not null references public.events (id) on delete cascade,
  email text not null,
  name text,
  guests int not null default 0,
  status text not null default 'going',
  created_at timestamptz not null default now(),
  unique (event_id, email)
);
alter table public.rsvps enable row level security;

-- ------------------------------------------------------------ giving
create table if not exists public.giving (
  id uuid primary key default gen_random_uuid(),
  church_id uuid not null references public.churches (id) on delete cascade,
  giver_email text,
  giver_name text,
  amount numeric(12,2) not null default 0,
  currency text not null default 'USD',
  method text not null default 'cash',
  purpose text not null default 'tithe',
  reference text,
  given_on date not null default current_date,
  recorded_by text,
  created_at timestamptz not null default now()
);
alter table public.giving enable row level security;

-- -------------------------------------------------------- recordings
create table if not exists public.recordings (
  id uuid primary key default gen_random_uuid(),
  church_id uuid not null references public.churches (id) on delete cascade,
  title text not null,
  url text not null,
  provider text not null default 'custom',
  session_id uuid references public.sessions (id) on delete set null,
  duration_min int,
  published boolean not null default true,
  created_at timestamptz not null default now()
);
alter table public.recordings enable row level security;

-- ============================================================
-- Policies
-- ============================================================

-- Staff-managed tables: readable by the church, writable by editors.
do $$
declare t text;
begin
  foreach t in array array[
    'groups','service_plans','service_plan_items',
    'volunteer_slots','volunteer_assignments',
    'attendance','follow_ups','recordings'
  ] loop
    if not exists (select 1 from pg_policy where polrelid = ('public.'||t)::regclass and polname = t||'_select') then
      execute format('create policy %I on public.%I for select using (church_id = public.my_church_id() or public.is_platform_admin())', t||'_select', t);
    end if;
    if not exists (select 1 from pg_policy where polrelid = ('public.'||t)::regclass and polname = t||'_write') then
      execute format('create policy %I on public.%I for all using (public.is_editor() and church_id = public.my_church_id()) with check (public.is_editor() and church_id = public.my_church_id())', t||'_write', t);
    end if;
    if not exists (select 1 from pg_policy where polrelid = ('public.'||t)::regclass and polname = t||'_platform_admin') then
      execute format('create policy %I on public.%I for all using (public.is_platform_admin()) with check (public.is_platform_admin())', t||'_platform_admin', t);
    end if;
  end loop;
end $$;

-- group_members: members may join/leave themselves; editors manage all.
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.group_members'::regclass and polname = 'group_members_select') then
    create policy "group_members_select" on public.group_members for select using (church_id = public.my_church_id() or public.is_platform_admin());
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.group_members'::regclass and polname = 'group_members_insert') then
    create policy "group_members_insert" on public.group_members for insert with check (church_id = public.my_church_id() and (profile_id = auth.uid() or public.is_editor()));
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.group_members'::regclass and polname = 'group_members_update') then
    create policy "group_members_update" on public.group_members for update using (church_id = public.my_church_id() and (profile_id = auth.uid() or public.is_editor())) with check (church_id = public.my_church_id());
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.group_members'::regclass and polname = 'group_members_delete') then
    create policy "group_members_delete" on public.group_members for delete using (church_id = public.my_church_id() and (profile_id = auth.uid() or public.is_editor()));
  end if;
end $$;

-- content_items: global published items plus your church's; editors write.
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.content_items'::regclass and polname = 'content_items_select') then
    create policy "content_items_select" on public.content_items for select using (
      public.is_platform_admin()
      or (scope = 'global' and state = 'published')
      or (church_id = public.my_church_id() and (state = 'published' or public.is_editor()))
    );
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.content_items'::regclass and polname = 'content_items_write') then
    create policy "content_items_write" on public.content_items for all using (public.is_editor() and church_id = public.my_church_id()) with check (public.is_editor() and church_id = public.my_church_id());
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.content_items'::regclass and polname = 'content_items_leader') then
    create policy "content_items_leader" on public.content_items for all using (
      group_id is not null and exists (
        select 1 from public.group_members gm
        where gm.group_id = content_items.group_id and gm.profile_id = auth.uid() and gm.member_role = 'leader'
      )
    ) with check (
      church_id = public.my_church_id() and group_id is not null and exists (
        select 1 from public.group_members gm
        where gm.group_id = content_items.group_id and gm.profile_id = auth.uid() and gm.member_role = 'leader'
      )
    );
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.content_items'::regclass and polname = 'content_items_platform_admin') then
    create policy "content_items_platform_admin" on public.content_items for all using (public.is_platform_admin()) with check (public.is_platform_admin());
  end if;
end $$;

-- prayers: private stays with the author (and editors); anyone may post.
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.prayers'::regclass and polname = 'prayers_select') then
    create policy "prayers_select" on public.prayers for select using (
      public.is_platform_admin()
      or (church_id = public.my_church_id()
        and (not is_private or author_email = auth.jwt() ->> 'email' or public.is_editor()))
    );
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.prayers'::regclass and polname = 'prayers_insert') then
    create policy "prayers_insert" on public.prayers for insert with check (church_id = public.my_church_id() and author_email = auth.jwt() ->> 'email');
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.prayers'::regclass and polname = 'prayers_update') then
    create policy "prayers_update" on public.prayers for update using (church_id = public.my_church_id() and (author_email = auth.jwt() ->> 'email' or public.is_editor())) with check (church_id = public.my_church_id());
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.prayers'::regclass and polname = 'prayers_delete') then
    create policy "prayers_delete" on public.prayers for delete using (church_id = public.my_church_id() and (author_email = auth.jwt() ->> 'email' or public.is_editor()));
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.prayers'::regclass and polname = 'prayers_platform_admin') then
    create policy "prayers_platform_admin" on public.prayers for all using (public.is_platform_admin()) with check (public.is_platform_admin());
  end if;
end $$;

-- rsvps: everyone in the church can see counts; you manage your own.
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.rsvps'::regclass and polname = 'rsvps_select') then
    create policy "rsvps_select" on public.rsvps for select using (church_id = public.my_church_id() or public.is_platform_admin());
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.rsvps'::regclass and polname = 'rsvps_insert') then
    create policy "rsvps_insert" on public.rsvps for insert with check (church_id = public.my_church_id() and email = auth.jwt() ->> 'email');
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.rsvps'::regclass and polname = 'rsvps_update') then
    create policy "rsvps_update" on public.rsvps for update using (church_id = public.my_church_id() and email = auth.jwt() ->> 'email') with check (church_id = public.my_church_id());
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.rsvps'::regclass and polname = 'rsvps_delete') then
    create policy "rsvps_delete" on public.rsvps for delete using (church_id = public.my_church_id() and (email = auth.jwt() ->> 'email' or public.is_editor()));
  end if;
end $$;

-- giving: leaders see all, a giver sees their own.
do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.giving'::regclass and polname = 'giving_select') then
    create policy "giving_select" on public.giving for select using (
      public.is_platform_admin()
      or (church_id = public.my_church_id() and (public.is_editor() or giver_email = auth.jwt() ->> 'email'))
    );
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.giving'::regclass and polname = 'giving_write') then
    create policy "giving_write" on public.giving for all using (public.is_editor() and church_id = public.my_church_id()) with check (public.is_editor() and church_id = public.my_church_id());
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.giving'::regclass and polname = 'giving_platform_admin') then
    create policy "giving_platform_admin" on public.giving for all using (public.is_platform_admin()) with check (public.is_platform_admin());
  end if;
end $$;

-- --------------------------------------------- helper: pray for a request
create or replace function public.pray_for(pid uuid)
returns void language sql security definer set search_path = public as $$
  update public.prayers set pray_count = pray_count + 1
  where id = pid and church_id = public.my_church_id();
$$;

-- ------------------------------------------------------------ indexes
create index if not exists content_church_idx on public.content_items (church_id, state, publish_at desc);
create index if not exists groups_church_idx on public.groups (church_id, name);
create index if not exists group_members_group_idx on public.group_members (group_id);
create index if not exists plans_church_idx on public.service_plans (church_id, plan_date desc);
create index if not exists plan_items_plan_idx on public.service_plan_items (plan_id, position);
create index if not exists slots_church_idx on public.volunteer_slots (church_id, service_date);
create index if not exists assignments_slot_idx on public.volunteer_assignments (slot_id);
create index if not exists attendance_church_idx on public.attendance (church_id, service_date desc);
create index if not exists followups_church_idx on public.follow_ups (church_id, stage);
create index if not exists prayers_church_idx on public.prayers (church_id, created_at desc);
create index if not exists rsvps_event_idx on public.rsvps (event_id);
create index if not exists giving_church_idx on public.giving (church_id, given_on desc);
create index if not exists recordings_church_idx on public.recordings (church_id, created_at desc);

-- --------------------------------------------------- realtime streams
do $$ begin
  if not exists (select 1 from pg_publication_tables where pubname = 'supabase_realtime' and schemaname = 'public' and tablename = 'content_items') then
    alter publication supabase_realtime add table public.content_items;
  end if;
  if not exists (select 1 from pg_publication_tables where pubname = 'supabase_realtime' and schemaname = 'public' and tablename = 'prayers') then
    alter publication supabase_realtime add table public.prayers;
  end if;
  if not exists (select 1 from pg_publication_tables where pubname = 'supabase_realtime' and schemaname = 'public' and tablename = 'attendance') then
    alter publication supabase_realtime add table public.attendance;
  end if;
end $$;


-- ============================================================
-- Domaine Church â€” public church directory
-- ------------------------------------------------------------
-- The register-church flow inserts a church BEFORE the account
-- is created (anonymous user). The INSERT passed (check true),
-- but the RETURNING SELECT was filtered by churches_select
-- (auth.uid() must be non-null), so PostgREST reported
-- "new row violates RLS policy". Churches are a public
-- directory, so make the directory readable by anyone.
-- ============================================================

drop policy if exists "churches_select" on public.churches;
create policy "churches_select" on public.churches for select using (true);

-- ============================================================
-- Domaine Church â€” ownership grants Church Admin
-- ------------------------------------------------------------
-- Fixes the onboarding flow: a person who registers a church
-- (owner_email) and belongs to that church is automatically its
-- Church Admin. No self-promotion loop, no admin needed later.
-- Safe to re-run.
-- ============================================================

-- Guard: the person who owns the church they belong to always
-- holds Church Admin, regardless of how the profile row is written.
create or replace function public.guard_profile_role()
returns trigger language plpgsql security definer set search_path = public as $$
begin
  if new.church_id is not null and exists (
    select 1 from public.churches c
    where c.id = new.church_id
      and lower(c.owner_email) = lower(new.email)
  ) then
    new.role := 'Church Admin';
    return new;
  end if;
  if not public.is_church_admin() then
    if tg_op = 'INSERT' then
      new.role := 'Member';
    elsif new.role is distinct from old.role then
      raise exception 'Only a church admin can change roles.';
    end if;
  end if;
  return new;
end $$;

-- Keep ownership changes in sync: if a church's owner list is set
-- (or changed) and the owner already belongs to the church, promote
-- them. Runs as the table owner (platform/manual) so it bypasses the
-- role-change guard for the owner while using the guard above too.
create or replace function public.promote_owner_profile()
returns trigger language plpgsql security definer set search_path = public as $$
begin
  if new.owner_email is not null then
    update public.profiles p
       set role = 'Church Admin'
     where p.church_id = new.id
       and lower(p.email) = lower(new.owner_email)
       and p.role <> 'Church Admin';
  end if;
  return new;
end $$;

drop trigger if exists profiles_guard_role on public.profiles;
create trigger profiles_guard_role
  before insert or update of role on public.profiles
  for each row execute function public.guard_profile_role();

drop trigger if exists churches_promote_owner on public.churches;
create trigger churches_promote_owner
  after insert or update of owner_email on public.churches
  for each row execute function public.promote_owner_profile();

-- ============================================================
-- Domaine Church â€” platform admin provisions churches + users
-- ------------------------------------------------------------
-- The platform admin is NOT a member of any church; these
-- SECURITY DEFINER RPCs let them create churches and full
-- accounts (auth user + profile + role + church link) from the
-- admin console, without needing the service-role key app-side.
-- Only the platform admin email may run them.
-- ============================================================

-- Create (or return) an auth user + profile in one step.
create or replace function public.admin_create_user(
  v_email text,
  v_password text,
  v_full_name text,
  v_role text,
  v_church_id uuid
) returns uuid
language plpgsql security definer set search_path = public, auth as $$
declare
  v_id uuid;
  v_low text;
begin
  if lower(auth.jwt() ->> 'email') not in (select lower(e) from public.platform_admin_emails e) then
    raise exception 'Platform admin required.';
  end if;

  v_low := lower(trim(v_email));
  select id into v_id from auth.users where lower(email) = v_low limit 1;
  if v_id is null then
    v_id := gen_random_uuid();
    insert into auth.users (
      instance_id, id, aud, role, email, encrypted_password, email_confirmed_at,
      raw_app_meta_data, raw_user_meta_data, created_at, updated_at, is_sso_user, is_anonymous
    ) values (
      '00000000-0000-0000-0000-000000000000',
      v_id, 'authenticated', 'authenticated', v_low,
      crypt(v_password, gen_salt('bf')), now(),
      '{"provider":"email","providers":["email"]}'::jsonb,
      jsonb_build_object('full_name', v_full_name),
      now(), now(), false, false
    );
    insert into auth.identities (
      id, user_id, provider_id, identity_data, provider, last_sign_in_at, created_at, updated_at, email
    ) values (
      v_id, v_id, v_id::text,
      jsonb_build_object('sub', v_id::text, 'email', v_low),
      'email', now(), now(), now(), v_low
    );
  end if;

  insert into public.profiles (id, email, full_name, role, church_id)
  values (v_id, v_low, v_full_name, coalesce(nullif(v_role, ''), 'Member'), v_church_id)
  on conflict (id) do update set
    full_name = excluded.full_name,
    role = excluded.role,
    church_id = excluded.church_id;

  return v_id;
end $$;

-- Register a church (and optionally its owner contact) on behalf.
create or replace function public.admin_create_church(
  v_name text,
  v_city text default null,
  v_country text default null,
  v_description text default null,
  v_contact_email text default null,
  v_owner_email text default null,
  v_owner_name text default null
) returns uuid
language plpgsql security definer set search_path = public as $$
declare
  v_id uuid;
  v_slug text;
begin
  if lower(auth.jwt() ->> 'email') not in (select lower(e) from public.platform_admin_emails e) then
    raise exception 'Platform admin required.';
  end if;

  v_slug := lower(trim(v_name));
  v_slug := regexp_replace(v_slug, '[^a-z0-9]+', '-', 'g');
  v_slug := trim(both '-' from v_slug);
  if v_slug = '' then v_slug := 'church'; end if;
  v_slug := v_slug || '-' || left(replace(gen_random_uuid()::text, '-', ''), 8);

  insert into public.churches (name, slug, city, country, description, contact_email, owner_email, owner_name)
  values (trim(v_name), v_slug, nullif(v_city, ''), nullif(v_country, ''), nullif(v_description, ''),
          nullif(v_contact_email, ''), nullif(v_owner_email, ''), nullif(v_owner_name, ''))
  returning id into v_id;

  return v_id;
end $$;

-- # Single platform admin login (secure)
-- One overall administrator account. The platform admin app only allows sign-in â€”
-- accounts are created by Domaine through startup setup (this migration) or manually.
-- Initial credentials were applied on first run of this migration and are not stored here.

create or replace function public.admin_set_password(v_email text, v_password text)
returns void
language plpgsql
security definer
set search_path = public, auth
as $$
declare
  v_creator text := nullif(current_setting('request.jwt.claims', true), '')::jsonb->>'email';
begin
  if v_creator is null or lower(v_creator) <> 'johnkamonyegitau@gmail.com' then
    raise exception 'Platform admin only';
  end if;
  update auth.users
     set encrypted_password = extensions.crypt(v_password, extensions.gen_salt('bf')),
         email_confirmed_at = coalesce(email_confirmed_at, now()),
         updated_at = now()
   where lower(email) = lower(v_email);
end
$$;