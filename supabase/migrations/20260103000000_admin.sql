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
