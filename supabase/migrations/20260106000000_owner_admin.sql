-- ============================================================
-- Domaine Church — ownership grants Church Admin
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