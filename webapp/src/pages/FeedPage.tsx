import { useEffect, useState } from 'react';
import { useAuth } from '../auth';
import { Card, Button, Modal, Field, Badge, Spinner, Empty, Avatar } from '../components/ui';
import { Icon } from '../components/icons';
import { fetchFeed, fetchPrayers, addPrayer, prayFor, setPrayerStatus, deletePrayer, fetchEventRsvps, saveRsvp, type ContentItem, type Prayer, type Rsvp } from '../lib/churchlife';
import { fetchEvents, type ChurchEvent } from '../lib/api';
import { useToast } from '../components/toast';
import { timeAgo, formatDate, todayISO } from '../utils';

export function FeedPage() {
  const { user, profile, canEdit } = useAuth();
  const toast = useToast();
  const [tab, setTab] = useState<'feed' | 'prayer' | 'events'>('feed');

  const [feed, setFeed] = useState<ContentItem[] | null>(null);
  const [prayers, setPrayers] = useState<Prayer[] | null>(null);
  const [events, setEvents] = useState<ChurchEvent[] | null>(null);
  const [rsvps, setRsvps] = useState<Rsvp[]>([]);
  const [ask, setAsk] = useState(false);
  const [prayText, setPrayText] = useState('');
  const [isPrivate, setIsPrivate] = useState(false);
  const [busy, setBusy] = useState(false);

  const email = user?.email || '';
  const name = profile?.full_name || email.split('@')[0];

  function load() {
    fetchFeed().then(setFeed).catch(() => setFeed([]));
    fetchPrayers().then(setPrayers).catch(() => setPrayers([]));
    fetchEvents().then(setEvents).catch(() => setEvents([]));
    fetchEventRsvps().then(setRsvps).catch(() => setRsvps([]));
  }
  useEffect(load, []); // eslint-disable-line react-hooks/exhaustive-deps

  async function postPrayer() {
    if (!prayText.trim()) return;
    setBusy(true);
    try {
      await addPrayer(prayText.trim(), email, name, isPrivate);
      setPrayText(''); setIsPrivate(false); setAsk(false);
      toast('Prayer shared. The church is praying with you.', 'success');
      fetchPrayers().then(setPrayers).catch(() => {});
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
    finally { setBusy(false); }
  }

  async function pray(p: Prayer) {
    try {
      await prayFor(p.id);
      setPrayers((prev) => (prev || []).map((x) => (x.id === p.id ? { ...x, pray_count: x.pray_count + 1 } : x)));
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
  }

  async function answer(p: Prayer) {
    try {
      await setPrayerStatus(p.id, p.status === 'answered' ? 'open' : 'answered');
      setPrayers((prev) => (prev || []).map((x) => (x.id === p.id ? { ...x, status: p.status === 'answered' ? 'open' : 'answered' } : x)));
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
  }

  async function rsvp(ev: ChurchEvent, status: string) {
    try {
      await saveRsvp(ev.id, email, name, status, 0);
      toast(`You replied ${status} for ${ev.title}.`, 'success');
      fetchEventRsvps().then(setRsvps).catch(() => {});
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
  }
  const myRsvp = (id: string) => rsvps.find((r) => r.event_id === id && r.email === email)?.status;
  const goingCount = (id: string) => rsvps.filter((r) => r.event_id === id && r.status === 'going').length;

  const upcoming = (events || []).filter((e) => (e.date || '') >= todayISO());

  return (
    <div className="stack">
      <Card title="Church life">
        <div className="tabs">
          <button className={tab === 'feed' ? 'active' : ''} onClick={() => setTab('feed')}>Feed</button>
          <button className={tab === 'prayer' ? 'active' : ''} onClick={() => setTab('prayer')}>Prayer wall</button>
          <button className={tab === 'events' ? 'active' : ''} onClick={() => setTab('events')}>Events</button>
        </div>
      </Card>

      {tab === 'feed' && (
        !feed ? <Spinner /> : feed.length === 0 ? (
          <Card><Empty title="Nothing published yet" sub="Announcements, devotionals and verses from your church will appear here." /></Card>
        ) : (
          <div className="stack">
            {feed.map((c) => (
              <Card key={c.id} title={c.title}>
                <div className="muted small">
                  {c.kind}{c.scope === 'global' ? ' · global' : ''} · {timeAgo(c.publish_at || c.created_at)}
                </div>
                {c.body && <p className="feed-body">{c.body}</p>}
                {c.link_url && <a className="feed-link" href={c.link_url} target="_blank" rel="noreferrer">Open link</a>}
              </Card>
            ))}
          </div>
        )
      )}

      {tab === 'prayer' && (
        <>
          <Card title="Share a prayer request"
            actions={<Button onClick={() => setAsk(true)}><Icon.Plus size={16} /> Add request</Button>}>
            <p className="muted small">Post publicly so the church can pray with you, or keep it private to the pastoral team.</p>
          </Card>
          {!prayers ? <Spinner /> : prayers.length === 0 ? (
            <Card><Empty title="No prayer requests yet" sub="Be the first to share." /></Card>
          ) : (
            <Card title="Prayer wall">
              <div className="list">
                {prayers.map((p) => (
                  <div key={p.id} className="list-row">
                    <Avatar name={p.author_name || p.author_email} size={34} />
                    <div className="list-main">
                      <div className="list-title">{p.author_name || p.author_email}
                        {p.is_private && <Badge tone="warn">private</Badge>}
                        {p.status === 'answered' && <Badge tone="success">answered</Badge>}
                      </div>
                      <div className="feed-body">{p.body}</div>
                      <div className="muted small">{timeAgo(p.created_at)} · {p.pray_count} praying</div>
                    </div>
                    <Button variant="soft" onClick={() => pray(p)}>🙏 Pray</Button>
                    {(canEdit || p.author_email === email) && (
                      <>
                        <Button variant="ghost" onClick={() => answer(p)}>{p.status === 'answered' ? 'Reopen' : 'Answered'}</Button>
                        <button className="icon-btn danger" onClick={() => confirm('Delete this request?') && deletePrayer(p.id).then(() => { toast('Deleted.', 'success'); fetchPrayers().then(setPrayers); })}><Icon.Trash size={16} /></button>
                      </>
                    )}
                  </div>
                ))}
              </div>
            </Card>
          )}
        </>
      )}

      {tab === 'events' && (
        !events ? <Spinner /> : upcoming.length === 0 ? (
          <Card><Empty title="No upcoming events" sub="Events added by leaders show here with RSVP." /></Card>
        ) : (
          <Card title="Upcoming events">
            <div className="list">
              {upcoming.map((e) => {
                const mine = myRsvp(e.id);
                return (
                  <div key={e.id} className="list-row">
                    <div className="list-main">
                      <div className="list-title">{e.title} {mine && <Badge tone="success">you: {mine}</Badge>}</div>
                      <div className="muted small">{formatDate(e.date)} · {e.start_time}{e.location ? ` · ${e.location}` : ''} · {goingCount(e.id)} going</div>
                      {e.description && <div className="muted small">{e.description}</div>}
                    </div>
                    <div className="row-actions">
                      <button className={`chip ${mine === 'going' ? 'on' : ''}`} onClick={() => rsvp(e, 'going')}>Going</button>
                      <button className={`chip ${mine === 'maybe' ? 'on' : ''}`} onClick={() => rsvp(e, 'maybe')}>Maybe</button>
                      <button className={`chip ${mine === 'no' ? 'on' : ''}`} onClick={() => rsvp(e, 'no')}>No</button>
                    </div>
                  </div>
                );
              })}
            </div>
          </Card>
        )
      )}

      <Modal open={ask} title="Share a prayer request" onClose={() => setAsk(false)}
        footer={<><Button variant="ghost" onClick={() => setAsk(false)}>Cancel</Button>
          <Button disabled={busy || !prayText.trim()} onClick={postPrayer}>{busy ? 'Posting…' : 'Post'}</Button></>}>
        <Field label="Your prayer request"><textarea rows={4} value={prayText} onChange={(e) => setPrayText(e.target.value)} /></Field>
        <label className="check-row">
          <input type="checkbox" checked={isPrivate} onChange={(e) => setIsPrivate(e.target.checked)} />
          <span>Keep private (only the pastoral team sees it)</span>
        </label>
      </Modal>
    </div>
  );
}