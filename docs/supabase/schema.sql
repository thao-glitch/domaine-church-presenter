-- ============================================================
-- Domaine Church Presenter - Supabase schema
-- ------------------------------------------------------------
-- How to use:
--   1) Create a free project at https://supabase.com
--   2) Open Project Settings -> API, copy the project URL and
--      the anon (public) key into supabase.ini next to the app.
--   3) Create user accounts under Authentication -> Users
--   4) Open the SQL editor and run this whole file.
--   5) In the Dashboard: add each staff member to the "profiles"
--      table with their sign-in email and role (e.g. "Pastor").
--      Roles that can edit: Bishop, Senior Pastor, Pastor,
--      Assistant Pastor, Elder, Deacon, Deaconess, Evangelist,
--      Minister, Worship Leader, Youth Leader.
--   6) Restart the app and log in.
-- ============================================================

create extension if not exists pgcrypto;

-- ---------- helper: is this user an editor? ----------
create or replace function public.is_editor()
returns boolean
language sql
stable
security definer
set search_path = public
as $$
  select exists (
    select 1
    from public.profiles p
    where p.email = auth.jwt() ->> 'email'
      and p.role in (
        'Bishop', 'Senior Pastor', 'Pastor', 'Assistant Pastor',
        'Elder', 'Deacon', 'Deaconess', 'Evangelist',
        'Minister', 'Worship Leader', 'Youth Leader'
      )
  );
$$;

-- ---------- profiles (maps a login email to a role) ----------
create table if not exists public.profiles (
  id uuid primary key references auth.users (id) on delete cascade,
  email text not null unique,
  full_name text,
  role text not null default 'Member',
  created_at timestamptz not null default now()
);

alter table public.profiles enable row level security;

drop policy if exists "profiles_select" on public.profiles;
create policy "profiles_select" on public.profiles
  for select using (true);

drop policy if exists "profiles_insert_own" on public.profiles;
create policy "profiles_insert_own" on public.profiles
  for insert with check (auth.uid() = id);

drop policy if exists "profiles_update_own" on public.profiles;
create policy "profiles_update_own" on public.profiles
  for update using (auth.uid() = id);

-- ---------- members ----------
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
create policy "members_select" on public.members
  for select using (true);

drop policy if exists "members_insert" on public.members;
create policy "members_insert" on public.members
  for insert with check (public.is_editor());

drop policy if exists "members_update" on public.members;
create policy "members_update" on public.members
  for update using (public.is_editor());

drop policy if exists "members_delete" on public.members;
create policy "members_delete" on public.members
  for delete using (public.is_editor());

-- ---------- services & schedules ----------
create table if not exists public.services (
  id uuid primary key default gen_random_uuid(),
  name text not null,
  type text,
  recurring boolean not null default false,
  weekday int,                    -- 0=Sunday .. 6=Saturday (for recurring)
  date date,                      -- one-off date (for non-recurring)
  start_time text,                -- HH:MM
  end_time text,                  -- HH:MM
  location text,
  created_at timestamptz not null default now()
);

alter table public.services enable row level security;

drop policy if exists "services_select" on public.services;
create policy "services_select" on public.services
  for select using (true);

drop policy if exists "services_insert" on public.services;
create policy "services_insert" on public.services
  for insert with check (public.is_editor());

drop policy if exists "services_update" on public.services;
create policy "services_update" on public.services
  for update using (public.is_editor());

drop policy if exists "services_delete" on public.services;
create policy "services_delete" on public.services
  for delete using (public.is_editor());

-- ---------- events & activities ----------
create table if not exists public.events (
  id uuid primary key default gen_random_uuid(),
  title text not null,
  category text,
  date date not null,
  start_time text,                -- HH:MM
  end_time text,                  -- HH:MM
  location text,
  description text,
  created_at timestamptz not null default now()
);

alter table public.events enable row level security;

drop policy if exists "events_select" on public.events;
create policy "events_select" on public.events
  for select using (true);

drop policy if exists "events_insert" on public.events;
create policy "events_insert" on public.events
  for insert with check (public.is_editor());

drop policy if exists "events_update" on public.events;
create policy "events_update" on public.events
  for update using (public.is_editor());

drop policy if exists "events_delete" on public.events;
create policy "events_delete" on public.events
  for delete using (public.is_editor());

-- ---------- sample data (safe to edit or delete) ----------
insert into public.services (name, type, recurring, weekday, start_time, end_time)
values
  ('Sunday Worship', 'Sunday Worship Service', true, 0, '10:00', '12:30'),
  ('Sunday Prayer', 'Prayer Meeting', true, 0, '08:30', '09:15'),
  ('Bible Study', 'Bible Study', true, 2, '18:30', '20:00'),
  ('Wednesday Prayer', 'Prayer Meeting', true, 3, '18:00', '19:30'),
  ('Youth Service', 'Youth Service', true, 5, '17:00', '19:00'),
  ('Choir Practice', 'Choir Practice', true, 4, '17:30', '19:00')
on conflict do nothing;

insert into public.events (title, category, date, start_time, end_time, location)
values
  ('New Year Service', 'Special Service', date_trunc('year', now())::date, '22:00', '00:30', 'Main Church'),
  ('Church Anniversary', 'Anniversary', date_trunc('year', now())::date
    + interval '6 months', '10:00', '14:00', 'Main Church')
on conflict do nothing;