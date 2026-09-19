import { useEffect, useState } from 'react';
import { useAuth } from '../auth';
import { Card, Button, Modal, Field, Badge, Spinner, Empty } from '../components/ui';
import { Icon } from '../components/icons';
import {
  fetchContent, fetchGroups, saveContent, setContentState, deleteContent,
  type ContentItem, type ContentKind, type ContentState, type Group
} from '../lib/churchlife';
import { useToast } from '../components/toast';
import { timeAgo } from '../utils';

const KINDS: { value: ContentKind; label: string }[] = [
  { value: 'announcement', label: 'Announcement' },
  { value: 'devotional', label: 'Devotional' },
  { value: 'sermon', label: 'Sermon' },
  { value: 'event', label: 'Event' },
  { value: 'verse', label: 'Verse of the day' },
  { value: 'prayer', label: 'Prayer point' },
  { value: 'poll', label: 'Poll' }
];
const CHANNELS = ['app', 'screen', 'sms', 'whatsapp', 'email'];
const AUDIENCES = ['all', 'leaders', 'members', 'group'];
const STATE_TONE: Record<ContentState, 'default' | 'success' | 'warn' | 'editor'> = {
  draft: 'default', scheduled: 'warn', published: 'success', archived: 'editor'
};

export function ContentPage() {
  const { profile, canEdit } = useAuth();
  const toast = useToast();
  const [items, setItems] = useState<ContentItem[] | null>(null);
  const [groups, setGroups] = useState<Group[]>([]);
  const [editing, setEditing] = useState<Partial<ContentItem> | 'new' | null>(null);
  const [busy, setBusy] = useState(false);

  function load() {
    fetchContent().then(setItems).catch((e) => toast(String(e), 'error'));
    fetchGroups().then(setGroups).catch(() => setGroups([]));
  }
  useEffect(load, []); // eslint-disable-line react-hooks/exhaustive-deps

  if (!canEdit) return <Empty title="Leaders only" sub="Ask your church admin for access to create and publish content." />;

  async function save() {
    if (!editing || editing === 'new') return;
    if (!editing.title?.trim()) { toast('Give the content a title.', 'error'); return; }
    setBusy(true);
    try {
      await saveContent({ ...editing, title: editing.title.trim(), created_by: editing.created_by || profile?.email || null });
      toast('Content saved.', 'success');
      setEditing(null); load();
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
    finally { setBusy(false); }
  }

  async function changeState(id: string, state: ContentState) {
    try {
      await setContentState(id, state);
      toast(state === 'published' ? 'Published to the church.' : `Marked ${state}.`, 'success');
      load();
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
  }

  const groupName = (id: string | null) => groups.find((g) => g.id === id)?.name;

  return (
    <div className="stack">
      <Card title="Content studio"
        actions={<Button onClick={() => setEditing({ kind: 'announcement', scope: 'church', audience: 'all', state: 'draft', channels: ['app'] })}>
          <Icon.Plus size={16} /> New content</Button>}>
        <p className="muted small">
          Create once, target an audience, pick channels, then publish now or schedule it. Published
          items flow to the member feed and can be shown on the stage screen.
        </p>
      </Card>

      {!items ? <Spinner /> : items.length === 0 ? (
        <Card><Empty title="No content yet" sub="Start with an announcement or this week's devotional." /></Card>
      ) : (
        <Card title="All content">
          <div className="list">
            {items.map((c) => (
              <div key={c.id} className="list-row">
                <div className="list-main">
                  <div className="list-title">
                    {c.title} <Badge tone={STATE_TONE[c.state]}>{c.state}</Badge>
                    {c.scope === 'global' && <Badge tone="leader">global</Badge>}
                  </div>
                  <div className="muted small">
                    {c.kind} · {c.audience}{c.group_id ? ` (${groupName(c.group_id) || 'group'})` : ''} ·
                    {' '}{c.channels.join(', ')} · {timeAgo(c.publish_at || c.created_at)}
                  </div>
                  {c.body && <div className="muted small content-body-preview">{c.body}</div>}
                </div>
                {c.state !== 'published'
                  ? <Button variant="soft" onClick={() => changeState(c.id, 'published')}>Publish</Button>
                  : <Button variant="ghost" onClick={() => changeState(c.id, 'archived')}>Archive</Button>}
                <RowActions
                  onEdit={() => setEditing(c)}
                  onDelete={() => confirm('Delete this content?') && deleteContent(c.id).then(() => { toast('Deleted.', 'success'); load(); }).catch((e) => toast(String(e), 'error'))} />
              </div>
            ))}
          </div>
        </Card>
      )}

      <Composer editing={editing} groups={groups} onClose={() => setEditing(null)} onSave={save} busy={busy} />
    </div>
  );
}

function RowActions({ onEdit, onDelete }: { onEdit: () => void; onDelete: () => void }) {
  return (
    <div className="row-actions">
      <button className="icon-btn" onClick={onEdit}><Icon.Edit size={16} /></button>
      <button className="icon-btn danger" onClick={onDelete}><Icon.Trash size={16} /></button>
    </div>
  );
}

function Composer({ editing, groups, onClose, onSave, busy }: {
  editing: Partial<ContentItem> | 'new' | null; groups: Group[];
  onClose: () => void; onSave: () => void; busy: boolean;
}) {
  const [d, setD] = useState<Partial<ContentItem>>({});
  useEffect(() => {
    setD(editing && editing !== 'new' ? { ...editing } : { kind: 'announcement', scope: 'church', audience: 'all', state: 'draft', channels: ['app'] });
  }, [editing]);

  const toggle = (ch: string) => {
    const set = new Set(d.channels || []);
    set.has(ch) ? set.delete(ch) : set.add(ch);
    setD({ ...d, channels: [...set] });
  };

  return (
    <Modal open={!!editing} title={editing === 'new' ? 'New content' : 'Edit content'} onClose={onClose}
      footer={<><Button variant="ghost" onClick={onClose}>Cancel</Button><Button disabled={busy} onClick={onSave}>{busy ? 'Saving…' : 'Save'}</Button></>}>
      <div className="row-2">
        <Field label="Type">
          <select value={d.kind || 'announcement'} onChange={(e) => setD({ ...d, kind: e.target.value as ContentKind })}>
            {KINDS.map((k) => <option key={k.value} value={k.value}>{k.label}</option>)}
          </select>
        </Field>
        <Field label="Audience">
          <select value={d.audience || 'all'} onChange={(e) => setD({ ...d, audience: e.target.value })}>
            {AUDIENCES.map((a) => <option key={a} value={a}>{a}</option>)}
          </select>
        </Field>
      </div>
      {d.audience === 'group' && (
        <Field label="Group">
          <select value={d.group_id || ''} onChange={(e) => setD({ ...d, group_id: e.target.value || null })}>
            <option value="">Choose a group…</option>
            {groups.map((g) => <option key={g.id} value={g.id}>{g.name}</option>)}
          </select>
        </Field>
      )}
      <Field label="Title"><input value={d.title || ''} onChange={(e) => setD({ ...d, title: e.target.value })} /></Field>
      <Field label="Message"><textarea rows={5} value={d.body || ''} onChange={(e) => setD({ ...d, body: e.target.value })} /></Field>
      <Field label="Link (optional)"><input value={d.link_url || ''} onChange={(e) => setD({ ...d, link_url: e.target.value })} placeholder="https://…" /></Field>
      <Field label="Channels" hint="Where this should appear once published.">
        <div className="chip-row">
          {CHANNELS.map((ch) => (
            <button key={ch} type="button" className={`chip ${(d.channels || []).includes(ch) ? 'on' : ''}`} onClick={() => toggle(ch)}>{ch}</button>
          ))}
        </div>
      </Field>
      <div className="row-2">
        <Field label="State">
          <select value={d.state || 'draft'} onChange={(e) => setD({ ...d, state: e.target.value as ContentState })}>
            <option value="draft">Draft</option>
            <option value="scheduled">Scheduled</option>
            <option value="published">Published</option>
            <option value="archived">Archived</option>
          </select>
        </Field>
        <Field label="Publish at">
          <input type="datetime-local" value={(d.publish_at || '').slice(0, 16)}
            onChange={(e) => setD({ ...d, publish_at: e.target.value ? new Date(e.target.value).toISOString() : null })} />
        </Field>
      </div>
      <Field label="Expires (optional)">
        <input type="date" value={(d.expires_at || '').slice(0, 10)}
          onChange={(e) => setD({ ...d, expires_at: e.target.value ? new Date(e.target.value).toISOString() : null })} />
      </Field>
    </Modal>
  );
}