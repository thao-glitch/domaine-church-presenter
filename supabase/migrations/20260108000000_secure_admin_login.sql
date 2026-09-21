-- # Single platform admin login (secure)
-- One overall administrator account. The platform admin app only allows sign-in —
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