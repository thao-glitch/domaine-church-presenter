-- ============================================================
-- Domaine Church — multi-church upgrade
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
    create policy "slides_select" on public.slides for select using (church_id = public.my_church_id());
  else
    drop policy "slides_select" on public.slides;
    create policy "slides_select" on public.slides for select using (church_id = public.my_church_id());
  end if;
  if not exists (select 1 from pg_policy where polrelid = 'public.slides'::regclass and polname = 'slides_write') then
    create policy "slides_write" on public.slides for all using (public.is_presenter() and church_id = public.my_church_id()) with check (public.is_presenter() and church_id = public.my_church_id());
  else
    drop policy "slides_write" on public.slides;
    create policy "slides_write" on public.slides for all using (public.is_presenter() and church_id = public.my_church_id()) with check (public.is_presenter() and church_id = public.my_church_id());
  end if;
end $$;

-- slides carry the church of their set
update public.slides s set church_id = ss.church_id
  from public.slide_sets ss where s.set_id = ss.id and s.church_id is null;

do $$ begin
  if not exists (select 1 from pg_policy where polrelid = 'public.slides'::regclass and polname = 'slides_select') then
    create policy "slides_select" on public.slides for select using (church_id = public.my_church_id());
  else
    drop policy "slides_select" on public.slides;
    create policy "slides_select" on public.slides for select using (church_id = public.my_church_id());
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