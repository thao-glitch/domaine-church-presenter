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
