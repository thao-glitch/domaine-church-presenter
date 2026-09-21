-- ============================================================
-- Domaine Church — public church directory
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