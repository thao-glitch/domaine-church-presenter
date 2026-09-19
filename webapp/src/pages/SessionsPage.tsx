import { useCallback, useEffect, useRef, useState } from 'react';
import { Room, RoomEvent, Track } from 'livekit-client';
import { useAuth } from '../auth';
import { Card, Button, Modal, Field, Spinner, Empty, Badge } from '../components/ui';
import { Icon } from '../components/icons';
import { fetchSessions, saveSession, deleteSession, requestLiveKitToken, type OnlineSession } from '../lib/api';
import { isConfigured } from '../config';
import { formatDateTime, todayISO } from '../utils';
import { useToast } from '../components/toast';

interface Tile { id: string; el: HTMLElement; }

export function SessionsPage() {
  const { canSchedule } = useAuth();
  const toast = useToast();
  const [sessions, setSessions] = useState<OnlineSession[] | null>(null);
  const [joining, setJoining] = useState<OnlineSession | null>(null);
  const [sched, setSched] = useState<Partial<OnlineSession> | 'new' | null>(null);
  const lk = isConfigured('livekit');
  const lkMissing = <Card><Empty title="LiveKit not set up" sub="Add your LiveKit server URL and deploy the token function (docs/supabase/LIVEKIT.md) in Setup." /></Card>;

  function load() { fetchSessions().then(setSessions).catch(() => setSessions([])); }
  useEffect(load, []);

  async function onSave() {
    if (!sched || sched === 'new') return;
    try {
      await saveSession({
        ...sched, title: sched.title || '',
        starts_at: (sched.starts_at || new Date().toISOString()),
        room: sched.room || `church-${Date.now().toString(36)}`
      });
      toast('Session scheduled.', 'success'); setSched(null); load();
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
  }

  const upcoming = (sessions || []).filter((s) => new Date(s.starts_at) > new Date(Date.now() - 3600000))
    .sort((a, b) => a.starts_at.localeCompare(b.starts_at));

  return (
    <div className="stack">
      <Card title="Online sessions"
        actions={canSchedule ? <Button onClick={() => setSched({ starts_at: new Date().toISOString() })}><Icon.Plus size={16} /> Schedule session</Button> : undefined}>
        <p className="muted small">Hold live video meetings and online church sessions. Anyone can join; leaders schedule them.</p>
      </Card>

      {!lk && lkMissing}

      {!sessions ? <Spinner label="Loading sessions…" />
       : sessions.length === 0 ? <Card><Empty title="No sessions" sub="Schedule the first online session." /></Card>
       : <Card title="Upcoming">
          {upcoming.length === 0 && <Empty title="Nothing upcoming" sub="Check the full list below, or schedule a session." />}
          <div className="list">
            {upcoming.map((s) => (
              <div key={s.id} className="list-row">
                <div className="list-main">
                  <div className="list-title">{s.title}</div>
                  <div className="muted small">{formatDateTime(s.starts_at)}{s.ends_at ? ` · until ${formatDateTime(s.ends_at)}` : ''} · host {s.host || '—'}</div>
                  {s.description && <div className="muted small">{s.description}</div>}
                </div>
                <Badge tone="warn">Live on {s.room}</Badge>
                <div className="row-actions">
                  <Button onClick={() => setJoining(s)}><Icon.Video size={15} /> Join</Button>
                  {canSchedule && <Button variant="ghost" onClick={() => setSched(s)}><Icon.Edit size={16} /></Button>}
                  {canSchedule && <button className="icon-btn danger" onClick={() => confirm('Delete this session?') && deleteSession(s.id).then(() => { toast('Session deleted.', 'success'); load(); }).catch((e) => toast(String(e), 'error'))}><Icon.Trash size={16} /></button>}
                </div>
              </div>
            ))}
          </div>
        </Card>}

      <Card title="Past sessions">
        <div className="list">
          {(sessions || []).filter((s) => !upcoming.includes(s)).slice(0, 8).map((s) => (
            <div key={s.id} className="list-row">
              <div className="list-main">
                <div className="list-title muted">{s.title}</div>
                <div className="muted small">{formatDateTime(s.starts_at)}</div>
              </div>
            </div>
          ))}
          {(sessions || []).filter((s) => !upcoming.includes(s)).length === 0 && <Empty title="No past sessions yet" />}
        </div>
      </Card>

      {joining && <RoomMeeting session={joining} onClose={() => setJoining(null)} />}

      <ScheduleModal editing={sched} onClose={() => setSched(null)} onSave={onSave} />
    </div>
  );
}

function ScheduleModal({ editing, onClose, onSave }: {
  editing: Partial<OnlineSession> | 'new' | null; onClose: () => void; onSave: () => void;
}) {
  const { profile, user } = useAuth();
  const [d, setD] = useState<Partial<OnlineSession>>({});
  useEffect(() => {
    setD(editing && editing !== 'new'
      ? { ...editing }
      : { starts_at: new Date().toISOString(), host: profile?.full_name || user?.email || '' });
  }, [editing]);
  return (
    <Modal open={!!editing} title={editing === 'new' ? 'Schedule an online session' : 'Edit session'} onClose={onClose}
      footer={<><Button variant="ghost" onClick={onClose}>Cancel</Button><Button disabled={!d.title} onClick={onSave}>Save</Button></>}>
      <Field label="Title"><input value={d.title || ''} onChange={(e) => setD({ ...d, title: e.target.value })} placeholder="e.g. Wednesday Bible study online" /></Field>
      <Field label="Starts (date + time)">
        <input type="datetime-local" value={d.starts_at ? toLocal(d.starts_at) : ''} onChange={(e) => setD({ ...d, starts_at: toIso(e.target.value) })} />
      </Field>
      <Field label="Host"><input value={d.host || ''} onChange={(e) => setD({ ...d, host: e.target.value })} /></Field>
      <Field label="Room name"><input value={d.room || ''} onChange={(e) => setD({ ...d, room: e.target.value })} placeholder="auto-generated if left empty" /></Field>
      <Field label="Description"><textarea value={d.description || ''} onChange={(e) => setD({ ...d, description: e.target.value })} rows={3} /></Field>
    </Modal>
  );
}

function toLocal(iso: string): string {
  try {
    const dt = new Date(iso);
    const p = (n: number) => String(n).padStart(2, '0');
    return `${dt.getFullYear()}-${p(dt.getMonth() + 1)}-${p(dt.getDate())}T${p(dt.getHours())}:${p(dt.getMinutes())}`;
  } catch { return ''; }
}
function toIso(local: string): string {
  return local ? new Date(local).toISOString() : new Date().toISOString();
}

function RoomMeeting({ session, onClose }: { session: OnlineSession; onClose: () => void }) {
  const { user, profile } = useAuth();
  const toast = useToast();
  const [status, setStatus] = useState<'connecting' | 'connected' | 'error'>('connecting');
  const [err, setErr] = useState('');
  const [tiles, setTiles] = useState<Tile[]>([]);
  const [localEl, setLocalEl] = useState<HTMLElement | null>(null);
  const [mic, setMic] = useState(true);
  const [cam, setCam] = useState(true);
  const roomRef = useRef<Room | null>(null);
  const localBoxRef = useRef<HTMLDivElement>(null);
  const tilesRef = useRef<Tile[]>([]);

  const removeTile = useCallback((id: string) => {
    tilesRef.current = tilesRef.current.filter((t) => t.id !== id);
    setTiles([...tilesRef.current]);
  }, []);

  async function connect() {
    const room = new Room({ adaptiveStream: true, dynacast: true });
    roomRef.current = room;
    try {
      const token = await requestLiveKitToken(
        session.room, user?.email || 'guest-' + Math.random().toString(36).slice(2, 8),
        profile?.full_name || user?.email || 'Member');
      room.on(RoomEvent.TrackSubscribed, (track, _pub, participant) => {
        if (track.kind === Track.Kind.Video) {
          const el = track.attach();
          el.dataset.ck = participant.identity;
          tilesRef.current = [...tilesRef.current.filter((t) => t.id !== participant.identity), { id: participant.identity, el }];
          setTiles([...tilesRef.current]);
        }
      });
      room.on(RoomEvent.TrackUnsubscribed, (track) => {
        track.detach().forEach((el) => el.remove());
        tilesRef.current = tilesRef.current.filter((t) => t.el.isConnected);
        setTiles([...tilesRef.current]);
      });
      room.on(RoomEvent.ParticipantDisconnected, (p) => removeTile(p.identity));
      room.on(RoomEvent.Disconnected, () => { setStatus(status === 'error' ? 'error' : 'error'); onClose(); });
      await room.connect((await import('../config')).resolveConfig().livekitUrl, token);
      await room.localParticipant.setCameraEnabled(true);
      await room.localParticipant.setMicrophoneEnabled(true);
      const camTrack = room.localParticipant.getTrackPublication(Track.Source.Camera);
      const el = camTrack?.track?.attach();
      if (el && localBoxRef.current) {
        localBoxRef.current.innerHTML = '';
        localBoxRef.current.appendChild(el);
      }
      setStatus('connected');
    } catch (e) {
      setErr(e instanceof Error ? e.message : String(e));
      setStatus('error');
    }
  }

  useEffect(() => {
    connect();
    return () => { try { roomRef.current?.disconnect(); } catch { /* noop */ } };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  async function toggleMic() {
    const next = !mic; setMic(next);
    await roomRef.current?.localParticipant.setMicrophoneEnabled(next).catch(() => setMic(!next));
  }
  async function toggleCam() {
    const next = !cam; setCam(next);
    await roomRef.current?.localParticipant.setCameraEnabled(next).catch(() => setCam(!next));
  }

  return (
    <Modal open title={session.title} onClose={onClose}>
      <div className="meeting-status muted small">{`${session.room} · online now`}</div>
      {status === 'connecting' && <Spinner label="Joining the session…" />}
      {status === 'connected' && (
        <>
          <div className="video-grid">
            <div className="video-tile" ref={localBoxRef}>
              <span className="video-name">You{localEl ? '' : ' — camera off'}</span>
            </div>
            {tiles.map((t) => (
              <div key={t.id} className="video-tile" ref={(node) => { if (node && t.el && !node.contains(t.el)) node.appendChild(t.el); }}>
                <span className="video-name">{t.id.split('@')[0]}</span>
              </div>
            ))}
            {tiles.length === 0 && <div className="video-tile empty-tile"><span className="muted">Waiting for others…</span></div>}
          </div>
          <div className="meeting-controls">
            <button className={`ctrl ${mic ? '' : 'off'}`} onClick={toggleMic}><Icon.Mic size={18} /></button>
            <button className={`ctrl ${cam ? '' : 'off'}`} onClick={toggleCam}><Icon.CamOff size={18} /></button>
            <button className="ctrl leave" onClick={onClose}><Icon.Stop size={18} /> Leave</button>
          </div>
        </>
      )}
      {status === 'error' && (
        <div className="form-error">{err}<div className="spacer" /><Button onClick={() => { setStatus('connecting'); setErr(''); connect(); }}>Try again</Button></div>
      )}
    </Modal>
  );
}