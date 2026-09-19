import { db, churchId } from './api';

// ---------------------------------------------------------- content engine
export type ContentKind = 'announcement' | 'devotional' | 'sermon' | 'event' | 'verse' | 'prayer' | 'poll';
export type ContentState = 'draft' | 'scheduled' | 'published' | 'archived';
export interface ContentItem {
  id: string;
  church_id: string | null;
  scope: 'global' | 'church' | 'group';
  kind: ContentKind;
  title: string;
  body: string;
  link_url: string | null;
  audience: string;
  group_id: string | null;
  state: ContentState;
  publish_at: string | null;
  expires_at: string | null;
  channels: string[];
  created_by: string | null;
  created_at: string;
  updated_at: string;
}

export async function fetchContent(): Promise<ContentItem[]> {
  const { data, error } = await db().from('content_items').select('*')
    .eq('church_id', churchId()).order('created_at', { ascending: false });
  if (error) throw new Error(error.message);
  return (data || []) as ContentItem[];
}

export async function fetchFeed(): Promise<ContentItem[]> {
  const { data, error } = await db().from('content_items').select('*')
    .eq('state', 'published')
    .or(`church_id.eq.${churchId()},scope.eq.global`)
    .order('publish_at', { ascending: false, nullsFirst: false })
    .order('created_at', { ascending: false });
  if (error) throw new Error(error.message);
  const now = Date.now();
  return ((data || []) as ContentItem[]).filter((c) => !c.expires_at || new Date(c.expires_at).getTime() > now);
}

export async function saveContent(c: Partial<ContentItem> & { title: string }) {
  const cid = churchId();
  const body: Record<string, unknown> = {
    church_id: cid,
    scope: c.scope || 'church',
    kind: c.kind || 'announcement',
    title: c.title,
    body: c.body || '',
    link_url: c.link_url || null,
    audience: c.audience || 'all',
    group_id: c.group_id || null,
    state: c.state || 'draft',
    publish_at: c.publish_at || null,
    expires_at: c.expires_at || null,
    channels: c.channels && c.channels.length ? c.channels : ['app'],
    updated_at: new Date().toISOString()
  };
  if (c.created_by) body.created_by = c.created_by;
  if (c.id) {
    const { error } = await db().from('content_items').update(body).eq('id', c.id).eq('church_id', cid);
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('content_items').insert(body);
    if (error) throw new Error(error.message);
  }
}

export async function setContentState(id: string, state: ContentState) {
  const patch: Record<string, unknown> = { state, updated_at: new Date().toISOString() };
  if (state === 'published') patch.publish_at = new Date().toISOString();
  const { error } = await db().from('content_items').update(patch).eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}

export async function deleteContent(id: string) {
  const { error } = await db().from('content_items').delete().eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}

// ---------------------------------------------------------------- groups
export interface Group {
  id: string; church_id: string; name: string; kind: string; description: string;
  leader_email: string | null; meeting_day: number | null; meeting_time: string | null;
  location: string | null; created_at: string;
}
export interface GroupMember {
  id: string; group_id: string; profile_id: string | null; name: string | null;
  member_role: string; created_at: string;
}

export async function fetchGroups(): Promise<Group[]> {
  const { data, error } = await db().from('groups').select('*').eq('church_id', churchId()).order('name');
  if (error) throw new Error(error.message);
  return (data || []) as Group[];
}
export async function saveGroup(g: Partial<Group> & { name: string }) {
  const cid = churchId();
  const body = {
    name: g.name, kind: g.kind || 'ministry', description: g.description || '',
    leader_email: g.leader_email || null, meeting_day: g.meeting_day ?? null,
    meeting_time: g.meeting_time || null, location: g.location || null
  };
  if (g.id) {
    const { error } = await db().from('groups').update(body).eq('id', g.id).eq('church_id', cid);
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('groups').insert({ ...body, church_id: cid });
    if (error) throw new Error(error.message);
  }
}
export async function deleteGroup(id: string) {
  const { error } = await db().from('groups').delete().eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}
export async function fetchGroupMembers(groupId: string): Promise<GroupMember[]> {
  const { data, error } = await db().from('group_members').select('*').eq('group_id', groupId).order('created_at');
  if (error) throw new Error(error.message);
  return (data || []) as GroupMember[];
}
export async function addGroupMember(groupId: string, profileId: string, name: string, memberRole = 'member') {
  const { error } = await db().from('group_members').insert({
    church_id: churchId(), group_id: groupId, profile_id: profileId, name, member_role: memberRole
  });
  if (error) throw new Error(error.message);
}
export async function removeGroupMember(id: string) {
  const { error } = await db().from('group_members').delete().eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}

// --------------------------------------------------------- service plans
export interface ServicePlan {
  id: string; church_id: string; title: string; plan_date: string;
  service_id: string | null; theme: string | null; notes: string | null; created_at: string;
}
export interface PlanItem {
  id: string; plan_id: string; position: number; item_type: string;
  title: string; person: string | null; duration_min: number | null; notes: string | null;
}

export async function fetchPlans(): Promise<ServicePlan[]> {
  const { data, error } = await db().from('service_plans').select('*').eq('church_id', churchId()).order('plan_date', { ascending: false });
  if (error) throw new Error(error.message);
  return (data || []) as ServicePlan[];
}
export async function savePlan(p: Partial<ServicePlan> & { title: string }) {
  const cid = churchId();
  const body = { title: p.title, plan_date: p.plan_date || new Date().toISOString().slice(0, 10), service_id: p.service_id || null, theme: p.theme || null, notes: p.notes || null };
  if (p.id) {
    const { error } = await db().from('service_plans').update(body).eq('id', p.id).eq('church_id', cid);
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('service_plans').insert({ ...body, church_id: cid });
    if (error) throw new Error(error.message);
  }
}
export async function deletePlan(id: string) {
  const { error } = await db().from('service_plans').delete().eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}
export async function fetchPlanItems(planId: string): Promise<PlanItem[]> {
  const { data, error } = await db().from('service_plan_items').select('*').eq('plan_id', planId).order('position');
  if (error) throw new Error(error.message);
  return (data || []) as PlanItem[];
}
export async function savePlanItem(it: Partial<PlanItem> & { plan_id: string; title: string }) {
  const body = {
    plan_id: it.plan_id, position: it.position ?? 0, item_type: it.item_type || 'custom',
    title: it.title, person: it.person || null, duration_min: it.duration_min ?? null, notes: it.notes || null
  };
  if (it.id) {
    const { error } = await db().from('service_plan_items').update(body).eq('id', it.id).eq('church_id', churchId());
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('service_plan_items').insert({ ...body, church_id: churchId() });
    if (error) throw new Error(error.message);
  }
}
export async function deletePlanItem(id: string) {
  const { error } = await db().from('service_plan_items').delete().eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}

// ------------------------------------------------------- volunteer roster
export interface VolunteerSlot {
  id: string; church_id: string; plan_id: string | null; role: string;
  title: string | null; service_date: string | null; needed: number; created_at: string;
}
export interface VolunteerAssignment {
  id: string; slot_id: string; profile_id: string | null; name: string | null; status: string; created_at: string;
}

export async function fetchSlots(): Promise<VolunteerSlot[]> {
  const { data, error } = await db().from('volunteer_slots').select('*').eq('church_id', churchId()).order('service_date', { ascending: false });
  if (error) throw new Error(error.message);
  return (data || []) as VolunteerSlot[];
}
export async function saveSlot(s: Partial<VolunteerSlot> & { role: string }) {
  const body = { role: s.role, title: s.title || null, service_date: s.service_date || null, needed: s.needed ?? 1, plan_id: s.plan_id || null };
  if (s.id) {
    const { error } = await db().from('volunteer_slots').update(body).eq('id', s.id).eq('church_id', churchId());
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('volunteer_slots').insert({ ...body, church_id: churchId() });
    if (error) throw new Error(error.message);
  }
}
export async function deleteSlot(id: string) {
  const { error } = await db().from('volunteer_slots').delete().eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}
export async function fetchAssignments(): Promise<VolunteerAssignment[]> {
  const { data, error } = await db().from('volunteer_assignments').select('*').eq('church_id', churchId()).order('created_at');
  if (error) throw new Error(error.message);
  return (data || []) as VolunteerAssignment[];
}
export async function saveAssignment(a: Partial<VolunteerAssignment> & { slot_id: string }) {
  const body = { slot_id: a.slot_id, profile_id: a.profile_id || null, name: a.name || null, status: a.status || 'invited' };
  if (a.id) {
    const { error } = await db().from('volunteer_assignments').update(body).eq('id', a.id).eq('church_id', churchId());
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('volunteer_assignments').insert({ ...body, church_id: churchId() });
    if (error) throw new Error(error.message);
  }
}
export async function deleteAssignment(id: string) {
  const { error } = await db().from('volunteer_assignments').delete().eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}

// ----------------------------------------------------------- attendance
export interface AttendanceRow {
  id: string; service_date: string; service_id: string | null; person_name: string;
  profile_id: string | null; status: string; checked_in_at: string;
}
export async function fetchAttendance(date: string): Promise<AttendanceRow[]> {
  const { data, error } = await db().from('attendance').select('*').eq('church_id', churchId()).eq('service_date', date).order('checked_in_at');
  if (error) throw new Error(error.message);
  return (data || []) as AttendanceRow[];
}
export async function addAttendance(personName: string, date: string, status = 'present', profileId: string | null = null) {
  const { error } = await db().from('attendance').insert({
    church_id: churchId(), person_name: personName, service_date: date, status, profile_id: profileId
  });
  if (error) throw new Error(error.message);
}
export async function deleteAttendance(id: string) {
  const { error } = await db().from('attendance').delete().eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}

// ----------------------------------------------------------- follow-ups
export interface FollowUp {
  id: string; person_name: string; phone: string | null; email: string | null;
  is_first_timer: boolean; stage: string; assigned_to: string | null; notes: string;
  created_at: string; updated_at: string;
}
export async function fetchFollowUps(): Promise<FollowUp[]> {
  const { data, error } = await db().from('follow_ups').select('*').eq('church_id', churchId()).order('created_at', { ascending: false });
  if (error) throw new Error(error.message);
  return (data || []) as FollowUp[];
}
export async function saveFollowUp(f: Partial<FollowUp> & { person_name: string }) {
  const body = {
    person_name: f.person_name, phone: f.phone || null, email: f.email || null,
    is_first_timer: f.is_first_timer ?? true, stage: f.stage || 'new',
    assigned_to: f.assigned_to || null, notes: f.notes || '', updated_at: new Date().toISOString()
  };
  if (f.id) {
    const { error } = await db().from('follow_ups').update(body).eq('id', f.id).eq('church_id', churchId());
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('follow_ups').insert({ ...body, church_id: churchId() });
    if (error) throw new Error(error.message);
  }
}
export async function deleteFollowUp(id: string) {
  const { error } = await db().from('follow_ups').delete().eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}

// ---------------------------------------------------------------- prayer
export interface Prayer {
  id: string; author_email: string; author_name: string | null; body: string;
  is_private: boolean; status: string; group_id: string | null; pray_count: number; created_at: string;
}
export async function fetchPrayers(): Promise<Prayer[]> {
  const { data, error } = await db().from('prayers').select('*').eq('church_id', churchId()).order('created_at', { ascending: false }).limit(200);
  if (error) throw new Error(error.message);
  return (data || []) as Prayer[];
}
export async function addPrayer(body: string, email: string, name: string, isPrivate: boolean) {
  const { error } = await db().from('prayers').insert({
    church_id: churchId(), body, author_email: email, author_name: name, is_private: isPrivate
  });
  if (error) throw new Error(error.message);
}
export async function setPrayerStatus(id: string, status: string) {
  const { error } = await db().from('prayers').update({ status }).eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}
export async function deletePrayer(id: string) {
  const { error } = await db().from('prayers').delete().eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}
export async function prayFor(id: string) {
  const { error } = await db().rpc('pray_for', { pid: id });
  if (error) throw new Error(error.message);
}

// ----------------------------------------------------------------- rsvps
export interface Rsvp {
  id: string; event_id: string; email: string; name: string | null; guests: number; status: string; created_at: string;
}
export async function fetchEventRsvps(): Promise<Rsvp[]> {
  const { data, error } = await db().from('rsvps').select('*').eq('church_id', churchId());
  if (error) throw new Error(error.message);
  return (data || []) as Rsvp[];
}
export async function saveRsvp(eventId: string, email: string, name: string, status: string, guests: number) {
  const { error } = await db().from('rsvps').upsert({
    church_id: churchId(), event_id: eventId, email, name, status, guests
  }, { onConflict: 'event_id,email' });
  if (error) throw new Error(error.message);
}

// ---------------------------------------------------------------- giving
export interface GivingRow {
  id: string; giver_email: string | null; giver_name: string | null; amount: number;
  currency: string; method: string; purpose: string; reference: string | null;
  given_on: string; recorded_by: string | null; created_at: string;
}
export async function fetchGiving(): Promise<GivingRow[]> {
  const { data, error } = await db().from('giving').select('*').eq('church_id', churchId()).order('given_on', { ascending: false });
  if (error) throw new Error(error.message);
  return (data || []) as GivingRow[];
}
export async function saveGiving(g: Partial<GivingRow> & { amount: number }) {
  const body = {
    giver_email: g.giver_email || null, giver_name: g.giver_name || null, amount: g.amount,
    currency: g.currency || 'USD', method: g.method || 'cash', purpose: g.purpose || 'tithe',
    reference: g.reference || null, given_on: g.given_on || new Date().toISOString().slice(0, 10),
    recorded_by: g.recorded_by || null
  };
  if (g.id) {
    const { error } = await db().from('giving').update(body).eq('id', g.id).eq('church_id', churchId());
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('giving').insert({ ...body, church_id: churchId() });
    if (error) throw new Error(error.message);
  }
}
export async function deleteGiving(id: string) {
  const { error } = await db().from('giving').delete().eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}

// ----------------------------------------------------------- recordings
export interface Recording {
  id: string; title: string; url: string; provider: string;
  session_id: string | null; duration_min: number | null; published: boolean; created_at: string;
}
export async function fetchRecordings(): Promise<Recording[]> {
  const { data, error } = await db().from('recordings').select('*').eq('church_id', churchId()).order('created_at', { ascending: false });
  if (error) throw new Error(error.message);
  return (data || []) as Recording[];
}
export async function saveRecording(r: Partial<Recording> & { title: string; url: string }) {
  const body = {
    title: r.title, url: r.url, provider: r.provider || 'custom',
    session_id: r.session_id || null, duration_min: r.duration_min ?? null, published: r.published ?? true
  };
  if (r.id) {
    const { error } = await db().from('recordings').update(body).eq('id', r.id).eq('church_id', churchId());
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('recordings').insert({ ...body, church_id: churchId() });
    if (error) throw new Error(error.message);
  }
}
export async function deleteRecording(id: string) {
  const { error } = await db().from('recordings').delete().eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}

// ------------------------------------------------- platform (global) content
export async function fetchGlobalContent(): Promise<ContentItem[]> {
  const { data, error } = await db().from('content_items').select('*')
    .eq('scope', 'global').order('created_at', { ascending: false });
  if (error) throw new Error(error.message);
  return (data || []) as ContentItem[];
}

export async function saveGlobalContent(c: Partial<ContentItem> & { title: string }) {
  const body: Record<string, unknown> = {
    church_id: null,
    scope: 'global',
    kind: c.kind || 'devotional',
    title: c.title,
    body: c.body || '',
    link_url: c.link_url || null,
    audience: c.audience || 'all',
    group_id: null,
    state: c.state || 'published',
    publish_at: c.publish_at || new Date().toISOString(),
    expires_at: c.expires_at || null,
    channels: c.channels && c.channels.length ? c.channels : ['app'],
    created_by: c.created_by || null,
    updated_at: new Date().toISOString()
  };
  if (c.id) {
    const { error } = await db().from('content_items').update(body).eq('id', c.id);
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('content_items').insert(body);
    if (error) throw new Error(error.message);
  }
}

export async function deleteGlobalContent(id: string) {
  const { error } = await db().from('content_items').delete().eq('id', id).eq('scope', 'global');
  if (error) throw new Error(error.message);
}

export async function countOf(table: string): Promise<number> {
  const { count, error } = await db().from(table).select('*', { count: 'exact', head: true });
  if (error) throw new Error(error.message);
  return count || 0;
}
