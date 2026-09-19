import type { RealtimeChannel } from '@supabase/supabase-js';
import { getClient } from './supabase';

export type Db = NonNullable<ReturnType<typeof getClient>>;

export function db(): Db {
  const sb = getClient();
  if (!sb) throw new Error('Supabase is not configured. Open the Setup screen.');
  return sb;
}

export interface Member { id: string; full_name: string; phone: string | null; email: string | null; role: string; created_at: string; }
export interface Service { id: string; name: string; type: string; recurring: boolean; weekday: number | null; date: string | null; start_time: string; end_time: string; location: string; created_at: string; }
export interface ChurchEvent { id: string; title: string; category: string; date: string; start_time: string; end_time: string; location: string; description: string; created_at: string; }
export interface FileRow { id: string; name: string; path: string; mime: string; size: number; category: string; uploaded_by: string; uploaded_by_name: string; created_at: string; }
export interface Message { id: string; channel_key: string; sender: string; sender_name: string; body: string; created_at: string; }
export interface Channel { id: string; key: string; name: string; kind: string; created_at: string; }
export interface OnlineSession { id: string; title: string; host: string; starts_at: string; ends_at: string | null; room: string; description: string; created_at: string; }
export interface SlideSet { id: string; title: string; owner: string; created_at: string; }
export interface Slide { id: string; set_id: string; idx: number; text: string; bg: string; created_at: string; }
export interface StageState { set_id: string | null; slide_idx: number; updated_at: string; }

// ---------------------------------------------------------- members
export async function fetchMembers(): Promise<Member[]> {
  const { data, error } = await db().from('members').select('*').order('role').order('full_name');
  if (error) throw new Error(error.message);
  return (data || []) as Member[];
}

export async function saveMember(m: Partial<Member> & { full_name: string }) {
  const body = { full_name: m.full_name, phone: m.phone || null, email: m.email || null, role: m.role || 'Member' };
  if (m.id) {
    const { error } = await db().from('members').update(body).eq('id', m.id);
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('members').insert(body);
    if (error) throw new Error(error.message);
  }
}

export async function deleteMember(id: string) {
  const { error } = await db().from('members').delete().eq('id', id);
  if (error) throw new Error(error.message);
}

// ------------------------------------------------------ services
export async function fetchServices(): Promise<Service[]> {
  const { data, error } = await db().from('services').select('*').order('name');
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
    const { error } = await db().from('services').update(body).eq('id', s.id);
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('services').insert(body);
    if (error) throw new Error(error.message);
  }
}

export async function deleteService(id: string) {
  const { error } = await db().from('services').delete().eq('id', id);
  if (error) throw new Error(error.message);
}

// -------------------------------------------------------- events
export async function fetchEvents(): Promise<ChurchEvent[]> {
  const { data, error } = await db().from('events').select('*').order('date', { ascending: false });
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
    const { error } = await db().from('events').update(body).eq('id', e.id);
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('events').insert(body);
    if (error) throw new Error(error.message);
  }
}

export async function deleteEvent(id: string) {
  const { error } = await db().from('events').delete().eq('id', id);
  if (error) throw new Error(error.message);
}

// ----------------------------------------------------------- files
export async function fetchFiles(): Promise<FileRow[]> {
  const { data, error } = await db().from('files').select('*').order('created_at', { ascending: false });
  if (error) throw new Error(error.message);
  return (data || []) as FileRow[];
}

export function fileUrl(path: string): string {
  return db().storage.from('uploads').getPublicUrl(path).data.publicUrl;
}

export async function uploadFile(file: File, category: string, email: string, name: string) {
  const sb = db();
  const path = `${Date.now()}_${file.name.replace(/[^a-zA-Z0-9._-]/g, '_')}`;
  const { error: upErr } = await sb.storage.from('uploads').upload(path, file, {
    cacheControl: '3600', contentType: file.type || 'application/octet-stream'
  });
  if (upErr) throw new Error(upErr.message);
  const { error: inErr } = await sb.from('files').insert({
    name: file.name, path, mime: file.type, size: file.size,
    category, uploaded_by: email, uploaded_by_name: name
  });
  if (inErr) throw new Error(inErr.message);
}

export async function deleteFileRow(f: FileRow): Promise<void> {
  const sb = db();
  const { error: stErr } = await sb.storage.from('uploads').remove([f.path]);
  if (stErr) throw new Error(stErr.message);
  const { error } = await sb.from('files').delete().eq('id', f.id);
  if (error) throw new Error(error.message);
}

// -------------------------------------------------- chat channels
export async function ensureChannels(): Promise<void> {
  const sb = db();
  const canned = [
    { key: 'general', name: 'General', kind: 'general' },
    { key: 'prayer', name: 'Prayer Pointers', kind: 'general' },
    { key: 'announcements', name: 'Announcements', kind: 'general' }
  ];
  for (const c of canned) {
    try {
      await sb.from('channels').upsert(c, { onConflict: 'key' });
    } catch {
      /* channel may already exist */
    }
  }
}

export async function fetchChannels(): Promise<Channel[]> {
  const { data, error } = await db().from('channels').select('*').order('kind').order('name');
  if (error) throw new Error(error.message);
  return (data || []) as Channel[];
}

export const dmKey = (a: string, b: string) => ['dm', a.toLowerCase(), b.toLowerCase()].sort().join('|');
export const roleKey = (role: string) => `role:${role}`;

export async function fetchMessages(key: string, limit = 60): Promise<Message[]> {
  const { data, error } = await db().from('messages')
    .select('*').eq('channel_key', key).order('created_at', { ascending: false }).limit(limit);
  if (error) throw new Error(error.message);
  return ((data || []) as Message[]).reverse();
}

export async function sendMessage(key: string, body: string, sender: string, senderName: string) {
  const { error } = await db().from('messages').insert({
    channel_key: key, body, sender, sender_name: senderName
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
  const { data, error } = await db().from('sessions').select('*').order('starts_at', { ascending: false });
  if (error) throw new Error(error.message);
  return (data || []) as OnlineSession[];
}

export async function saveSession(s: Partial<OnlineSession> & { title: string; starts_at: string; room: string }) {
  const body = {
    title: s.title, host: s.host || '', starts_at: s.starts_at, ends_at: s.ends_at || null,
    room: s.room, description: s.description || ''
  };
  if (s.id) {
    const { error } = await db().from('sessions').update(body).eq('id', s.id);
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('sessions').insert(body);
    if (error) throw new Error(error.message);
  }
}

export async function deleteSession(id: string) {
  const { error } = await db().from('sessions').delete().eq('id', id);
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
  const { data, error } = await db().from('slide_sets').select('*').order('created_at', { ascending: false });
  if (error) throw new Error(error.message);
  return (data || []) as SlideSet[];
}

export async function createSlideSet(title: string, owner: string): Promise<SlideSet> {
  const { data, error } = await db().from('slide_sets').insert({ title, owner }).select().single();
  if (error) throw new Error(error.message);
  return data as SlideSet;
}

export async function deleteSlideSet(id: string): Promise<void> {
  const { error } = await db().from('slide_sets').delete().eq('id', id);
  if (error) throw new Error(error.message);
}

export async function fetchSlides(setId: string): Promise<Slide[]> {
  const { data, error } = await db().from('slides').select('*').eq('set_id', setId).order('idx');
  if (error) throw new Error(error.message);
  return (data || []) as Slide[];
}

export async function saveSlide(sl: Partial<Slide> & { set_id: string; text?: string; bg?: string }): Promise<void> {
  const body = { set_id: sl.set_id, text: sl.text || '', bg: sl.bg || '#0b0f1a', idx: sl.idx ?? 0 };
  if (sl.id) {
    const { error } = await db().from('slides').update(body).eq('id', sl.id);
    if (error) throw new Error(error.message);
  } else {
    const { error } = await db().from('slides').insert(body);
    if (error) throw new Error(error.message);
  }
}

export async function deleteSlide(id: string): Promise<void> {
  const { error } = await db().from('slides').delete().eq('id', id);
  if (error) throw new Error(error.message);
}

export async function getStage(): Promise<StageState> {
  const { data, error } = await db().from('stage').select('*').eq('id', 1).maybeSingle();
  if (error) throw new Error(error.message);
  if (!data) {
    try {
      await db().from('stage').insert({ id: 1, set_id: null, slide_idx: 0 });
    } catch {
      /* race: already inserted */
    }
    return { set_id: null, slide_idx: 0, updated_at: new Date().toISOString() };
  }
  return data as StageState;
}

export async function setStage(setId: string | null, slideIdx: number): Promise<void> {
  const { error } = await db().from('stage').update({ set_id: setId, slide_idx: slideIdx }).eq('id', 1);
  if (error) throw new Error(error.message);
}