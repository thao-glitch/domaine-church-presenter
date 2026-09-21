-- ============================================================
-- Domaine Church — platform admin provisions churches + users
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