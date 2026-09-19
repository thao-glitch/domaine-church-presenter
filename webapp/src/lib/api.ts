import type { RealtimeChannel } from '@supabase/supabase-js';
import { getClient } from './supabase';

export type Db = NonNullable<ReturnType<typeof getClient>>;

export function db(): Db {
  const sb = getClient();
  if (!sb) throw new Error('Supabase is not configured. Open the Setup screen.');
  return sb;
}

// -------------------------------------------------------- church scope
// Every query runs inside one church. The auth provider sets the scope when
// a profile loads; callers never pass the id around.
let activeChurch: string | null = null;

export function setChurchScope(id: string | null) {
  activeChurch = id;
}
export function churchId(): string {
  const id = activeChurch;
  if (!id) throw new Error('Your account is not linked to a church yet. Ask your church admin, or check with leadership.');
  return id;
}

export interface Member { id: string; full_name: string; phone: string | null; email: string | null; role: string; church_id?: string; created_at: string; }
export interface Service { id: string; name: string; type: string; recurring: boolean; weekday: number | null; date: string | null; start_time: string; end_time: string; location: string; church_id?: string; created_at: string; }
export interface ChurchEvent { id: string; title: string; category: string; date: string; start_time: string; end_time: string; location: string; description: string; church_id?: string; created_at: string; }
export interface FileRow { id: string; name: string; path: string; mime: string; size: number; category: string; uploaded_by: string; uploaded_by_name: string; church_id?: string; created_at: string; }
export interface Message { id: string; channel_key: string; sender: string; sender_name: string; body: string; church_id?: string; created_at: string; }
export interface Channel { id: string; key: string; name: string; kind: string; church_id?: string; created_at: string; }
export interface OnlineSession { id: string; title: string; host: string; starts_at: string; ends_at: string | null; room: string; description: string; church_id?: string; created_at: string; }
export interface SlideSet { id: string; title: string; owner: string; church_id?: string; created_at: string; }
export interface Slide { id: string; set_id: string; idx: number; text: string; bg: string; church_id?: string; created_at: string; }
export interface StageState { set_id: string | null; slide_idx: number; updated_at: string; }
export interface Church { id: string; name: string; slug: string; city: string | null; country: string | null; description: string | null; contact_email: string | null; owner_email: string | null; owner_name: string | null; created_at: string; }
export interface ChurchProfile { id: string; email: string; full_name: string | null; role: string; }

// ----------------------------------------------------------- churches
export async function fetchChurches(): Promise<Church[]> {
  const { data, error } = await db().from('churches').select('id,name,city,country,slug').order('name');
  if (error) throw new Error(error.message);
  return (data || []) as Church[];
}

export async function fetchChurch(id: string): Promise<Church | null> {
  const { data, error } = await db().from('churches').select('*').eq('id', id).maybeSingle();
  if (error) throw new Error(error.message);
  return (data || null) as Church | null;
}

function slugify(name: string): string {
  const base = name.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-+|-+$/g, '').slice(0, 36);
  return (base || 'church') + '-' + Math.random().toString(36).slice(2, 6);
}

export async function registerChurch(input: {
  name: string; city?: string; country?: string; description?: string;
  contact_email?: string; owner_email: string; owner_name?: string;
}): Promise<Church> {
  const { data, error } = await db().from('churches').insert({
    name: input.name.trim(), slug: slugify(input.name),
    city: input.city || null, country: input.country || null,
    description: input.description || null,
    contact_email: input.contact_email || null,
    owner_email: input.owner_email, owner_name: input.owner_name || null
  }).select().single();
  if (error) throw new Error(error.message);
  return data as Church;
}

export async function saveChurch(id: string, patch: Partial<Church>): Promise<void> {
  const body: Record<string, unknown> = {};
  if (patch.name !== undefined) body.name = patch.name;
  if (patch.city !== undefined) body.city = patch.city;
  if (patch.country !== undefined) body.country = patch.country;
  if (patch.description !== undefined) body.description = patch.description;
  if (patch.contact_email !== undefined) body.contact_email = patch.contact_email;
  const { error } = await db().from('churches').update(body).eq('id', id);
  if (error) throw new Error(error.message);
}

export async function fetchChurchProfiles(): Promise<ChurchProfile[]> {
  const { data, error } = await db().from('profiles')
    .select('id,email,full_name,role').eq('church_id', churchId()).order('role').order('full_name');
  if (error) throw new Error(error.message);
  return (data || []) as ChurchProfile[];
}

export async function setProfileRole(userId: string, role: string): Promise<void> {
  const { error } = await db().from('profiles').update({ role }).eq('id', userId);
  if (error) throw new Error(error.message);
}

export async function deleteProfile(userId: string): Promise<void> {
  const { error } = await db().from('profiles').delete().eq('id', userId);
  if (error) throw new Error(error.message);
}

export async function deleteChurch(id: string): Promise<void> {
  const { error } = await db().from('churches').delete().eq('id', id);
  if (error) throw new Error(error.message);
}

export interface AdminProfile { id: string; email: string; full_name: string | null; role: string; church_id: string | null; created_at: string; }

export async function fetchAllProfiles(): Promise<AdminProfile[]> {
  const { data, error } = await db().from('profiles')
    .select('id,email,full_name,role,church_id,created_at').order('created_at', { ascending: false });
  if (error) throw new Error(error.message);
  return (data || []) as AdminProfile[];
}

export async function fetchAdminChurches(): Promise<Church[]> {
  const { data, error } = await db().from('churches').select('*').order('created_at');
  if (error) throw new Error(error.message);
  return (data || []) as Church[];
}

// ---------------------------------------------------------- members
export async function fetchMembers(): Promise<Member[]> {
  const { data, error } = await db().from('members').select('*').eq('church_id', churchId()).order('role').order('full_name');
  if (error) throw new Error(error.message);
  return (data || []) as Member[];
}

export async function saveMember(m: Partial<Member> & { full_name: string }) {
  const body = { full_name: m.full_name, phone: m.phone || null, email: m.email || null, role: m.role || 'Member' };
  if (m.id) {
    const { error } = await db().from('members').update(body).eq('id', m.id).eq('church_id', churchId());
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('members').insert({ ...body, church_id: churchId() });
    if (error) throw new Error(error.message);
  }
}

export async function deleteMember(id: string) {
  const { error } = await db().from('members').delete().eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}

// ------------------------------------------------------ services
export async function fetchServices(): Promise<Service[]> {
  const { data, error } = await db().from('services').select('*').eq('church_id', churchId()).order('name');
  if (error) throw new Error(error.message);
  return (data || []) as Service[];
}

export async function saveService(s: Partial<Service> & { name: string }) {
  const body = {
    name: s.name, type: s.type || '', recurring: !!s.recurring,
    weekday: s.recurring ? (s.weekday ?? 0) : null,
    date: s.recurring ? null : (s.date || null),
    start_time: s.start_time || '', end_time: s.end_time || '', location: s.location || ''
  };
  if (s.id) {
    const { error } = await db().from('services').update(body).eq('id', s.id).eq('church_id', churchId());
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('services').insert({ ...body, church_id: churchId() });
    if (error) throw new Error(error.message);
  }
}

export async function deleteService(id: string) {
  const { error } = await db().from('services').delete().eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}

// -------------------------------------------------------- events
export async function fetchEvents(): Promise<ChurchEvent[]> {
  const { data, error } = await db().from('events').select('*').eq('church_id', churchId()).order('date', { ascending: false });
  if (error) throw new Error(error.message);
  return (data || []) as ChurchEvent[];
}

export async function saveEvent(e: Partial<ChurchEvent> & { title: string; date: string }) {
  const body = {
    title: e.title, category: e.category || 'Special Service', date: e.date,
    start_time: e.start_time || '', end_time: e.end_time || '',
    location: e.location || '', description: e.description || ''
  };
  if (e.id) {
    const { error } = await db().from('events').update(body).eq('id', e.id).eq('church_id', churchId());
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('events').insert({ ...body, church_id: churchId() });
    if (error) throw new Error(error.message);
  }
}

export async function deleteEvent(id: string) {
  const { error } = await db().from('events').delete().eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}

// ----------------------------------------------------------- files
export async function fetchFiles(): Promise<FileRow[]> {
  const { data, error } = await db().from('files').select('*').eq('church_id', churchId()).order('created_at', { ascending: false });
  if (error) throw new Error(error.message);
  return (data || []) as FileRow[];
}

export function fileUrl(path: string): string {
  return db().storage.from('uploads').getPublicUrl(path).data.publicUrl;
}

export async function uploadFile(file: File, category: string, email: string, name: string) {
  const sb = db();
  const cid = churchId();
  const safe = file.name.replace(/[^a-zA-Z0-9._-]/g, '_');
  const path = `${cid}/${Date.now()}_${safe}`;
  const { error: upErr } = await sb.storage.from('uploads').upload(path, file, {
    cacheControl: '3600', contentType: file.type || 'application/octet-stream'
  });
  if (upErr) throw new Error(upErr.message);
  const { error: inErr } = await sb.from('files').insert({
    name: file.name, path, mime: file.type, size: file.size,
    category, uploaded_by: email, uploaded_by_name: name, church_id: cid
  });
  if (inErr) throw new Error(inErr.message);
}

export async function deleteFileRow(f: FileRow): Promise<void> {
  const sb = db();
  const { error: stErr } = await sb.storage.from('uploads').remove([f.path]);
  if (stErr) throw new Error(stErr.message);
  const { error } = await sb.from('files').delete().eq('id', f.id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}

// -------------------------------------------------- chat channels
export async function ensureChannels(): Promise<void> {
  const sb = db();
  const id = churchId();
  const canned = [
    { key: 'general', name: 'General', kind: 'general' },
    { key: 'prayer', name: 'Prayer Pointers', kind: 'general' },
    { key: 'announcements', name: 'Announcements', kind: 'general' }
  ];
  const { data } = await sb.from('channels').select('key').eq('church_id', id);
  const have = new Set((data || []).map((c) => (c as { key: string }).key));
  for (const c of canned) {
    if (have.has(c.key)) continue;
    try {
      await sb.from('channels').insert({ ...c, church_id: id });
    } catch {
      /* another tab raced us */
    }
  }
}

export async function fetchChannels(): Promise<Channel[]> {
  const { data, error } = await db().from('channels').select('*').eq('church_id', churchId()).order('kind').order('name');
  if (error) throw new Error(error.message);
  return (data || []) as Channel[];
}

export const dmKey = (a: string, b: string) => ['dm', a.toLowerCase(), b.toLowerCase()].sort().join('|');
export const roleKey = (role: string) => `role:${role}`;

export async function fetchMessages(key: string, limit = 60): Promise<Message[]> {
  const { data, error } = await db().from('messages')
    .select('*').eq('channel_key', key).eq('church_id', churchId())
    .order('created_at', { ascending: false }).limit(limit);
  if (error) throw new Error(error.message);
  return ((data || []) as Message[]).reverse();
}

export async function sendMessage(key: string, body: string, sender: string, senderName: string) {
  const { error } = await db().from('messages').insert({
    channel_key: key, body, sender, sender_name: senderName, church_id: churchId()
  });
  if (error) throw new Error(error.message);
}

export function subscribeMessages(key: string, onInsert: (m: Message) => void): () => void {
  const sb = db();
  const ch: RealtimeChannel = sb
    .channel(`messages:${key}`)
    .on('postgres_changes',
      { event: 'INSERT', schema: 'public', table: 'messages', filter: `channel_key=eq.${key}` },
      (payload) => onInsert(payload.new as Message))
    .subscribe();
  return () => { sb.removeChannel(ch); };
}

// ------------------------------------------------------- sessions
export async function fetchSessions(): Promise<OnlineSession[]> {
  const { data, error } = await db().from('sessions').select('*').eq('church_id', churchId()).order('starts_at', { ascending: false });
  if (error) throw new Error(error.message);
  return (data || []) as OnlineSession[];
}

export async function saveSession(s: Partial<OnlineSession> & { title: string; starts_at: string; room: string }) {
  const cid = churchId();
  const room = s.room.startsWith(cid.slice(0, 8)) ? s.room : `${cid.slice(0, 8)}-${s.room}`;
  const body = {
    title: s.title, host: s.host || '', starts_at: s.starts_at, ends_at: s.ends_at || null,
    room, description: s.description || ''
  };
  if (s.id) {
    const { error } = await db().from('sessions').update(body).eq('id', s.id).eq('church_id', cid);
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('sessions').insert({ ...body, church_id: cid });
    if (error) throw new Error(error.message);
  }
}

export async function deleteSession(id: string) {
  const { error } = await db().from('sessions').delete().eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}

export async function requestLiveKitToken(room: string, identity: string, name: string): Promise<string> {
  const url = (await import('../config')).resolveConfig().livekitTokenUrl;
  const res = await fetch(url, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ room, identity, name })
  });
  if (!res.ok) throw new Error(`LiveKit token request failed (${res.status})`);
  const data = await res.json();
  if (!data.token) throw new Error('No token returned. Is the edge function deployed?');
  return data.token as string;
}

// ----------------------------------------------------------- stage
export async function fetchSlideSets(): Promise<SlideSet[]> {
  const { data, error } = await db().from('slide_sets').select('*').eq('church_id', churchId()).order('created_at', { ascending: false });
  if (error) throw new Error(error.message);
  return (data || []) as SlideSet[];
}

export async function createSlideSet(title: string, owner: string): Promise<SlideSet> {
  const { data, error } = await db().from('slide_sets').insert({ title, owner, church_id: churchId() }).select().single();
  if (error) throw new Error(error.message);
  return data as SlideSet;
}

export async function deleteSlideSet(id: string): Promise<void> {
  const { error } = await db().from('slide_sets').delete().eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}

export async function fetchSlides(setId: string): Promise<Slide[]> {
  const { data, error } = await db().from('slides').select('*').eq('set_id', setId).eq('church_id', churchId()).order('idx');
  if (error) throw new Error(error.message);
  return (data || []) as Slide[];
}

export async function saveSlide(sl: Partial<Slide> & { set_id: string; text?: string; bg?: string }): Promise<void> {
  const body = { set_id: sl.set_id, text: sl.text || '', bg: sl.bg || '#0b0f1a', idx: sl.idx ?? 0, church_id: churchId() };
  if (sl.id) {
    const { error } = await db().from('slides').update(body).eq('id', sl.id).eq('church_id', churchId());
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('slides').insert(body);
    if (error) throw new Error(error.message);
  }
}

export async function deleteSlide(id: string): Promise<void> {
  const { error } = await db().from('slides').delete().eq('id', id).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}

export async function getStage(): Promise<StageState> {
  const cid = churchId();
  const { data, error } = await db().from('stage').select('*').eq('church_id', cid).maybeSingle();
  if (error) throw new Error(error.message);
  if (!data) {
    try {
      await db().from('stage').upsert({ id: 1, church_id: cid, set_id: null, slide_idx: 0 }, { onConflict: 'church_id' });
    } catch {
      /* race: another tab created it */
    }
    return { set_id: null, slide_idx: 0, updated_at: new Date().toISOString() };
  }
  return data as StageState;
}

export async function setStage(setId: string | null, slideIdx: number): Promise<void> {
  const { error } = await db().from('stage').update({ set_id: setId, slide_idx: slideIdx }).eq('church_id', churchId());
  if (error) throw new Error(error.message);
}