import { useEffect, useState } from 'react';
import { useAuth } from '../auth';
import { Card, Button, Modal, Field, Badge, Spinner, Empty } from '../components/ui';
import { Icon } from '../components/icons';
import { fetchRecordings, saveRecording, deleteRecording, type Recording } from '../lib/churchlife';
import { useToast } from '../components/toast';
import { formatDate } from '../utils';

const PROVIDERS = ['youtube', 'facebook', 'livekit', 'custom'];

export function RecordingsPage() {
  const { canEdit } = useAuth();
  const toast = useToast();
  const [rows, setRows] = useState<Recording[] | null>(null);
  const [modal, setModal] = useState<Partial<Recording> | null>(null);
  const [busy, setBusy] = useState(false);

  function load() {
    fetchRecordings().then(setRows).catch(() => setRows([]));
  }
  useEffect(load, []); // eslint-disable-line react-hooks/exhaustive-deps

  async function save() {
    if (!modal || !modal.title?.trim() || !modal.url?.trim()) return;
    setBusy(true);
    try {
      await saveRecording({ ...modal, title: modal.title.trim(), url: modal.url.trim() });
      toast('Recording saved.', 'success'); setModal(null); load();
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
    finally { setBusy(false); }
  }

  const visible = (rows || []).filter((r) => canEdit || r.published);

  return (
    <div className="stack">
      <Card title="Sermon archive"
        actions={canEdit ? <Button onClick={() => setModal({ provider: 'youtube', published: true })}><Icon.Plus size={16} /> Add recording</Button> : undefined}>
        <p className="muted small">Paste recorded streams (YouTube, Facebook, or your LiveKit recordings) so the church can watch again anytime.</p>
      </Card>

      {!rows ? <Spinner /> : visible.length === 0 ? <Empty title="No recordings yet" sub="Add a link after your next service." /> : (
        <Card title="All recordings">
          <div className="list">
            {visible.map((r) => (
              <div key={r.id} className="list-row">
                <div className="list-main">
                  <div className="list-title">{r.title} {!r.published && <Badge tone="warn">unpublished</Badge>}</div>
                  <div className="muted small">{r.provider} · {formatDate(r.created_at)}{r.duration_min ? ` · ${r.duration_min} min` : ''}</div>
                </div>
                <a className="btn btn-soft" href={r.url} target="_blank" rel="noreferrer">Watch</a>
                {canEdit && (
                  <div className="row-actions">
                    <button className="icon-btn" onClick={() => setModal(r)}><Icon.Edit size={16} /></button>
                    <button className="icon-btn danger" onClick={() => confirm('Delete this recording?') && deleteRecording(r.id).then(() => { toast('Deleted.', 'success'); load(); })}><Icon.Trash size={16} /></button>
                  </div>
                )}
              </div>
            ))}
          </div>
        </Card>
      )}

      <Modal open={!!modal} title={modal?.id ? 'Edit recording' : 'Add recording'} onClose={() => setModal(null)}
        footer={<><Button variant="ghost" onClick={() => setModal(null)}>Cancel</Button><Button disabled={busy || !modal?.title?.trim() || !modal?.url?.trim()} onClick={save}>Save</Button></>}>
        <Field label="Title"><input value={modal?.title || ''} onChange={(e) => setModal({ ...(modal as object), title: e.target.value })} /></Field>
        <Field label="Video URL"><input value={modal?.url || ''} onChange={(e) => setModal({ ...(modal as object), url: e.target.value })} placeholder="https://youtu.be/…" /></Field>
        <div className="row-2">
          <Field label="Provider">
            <select value={modal?.provider || 'youtube'} onChange={(e) => setModal({ ...(modal as object), provider: e.target.value })}>
              {PROVIDERS.map((p) => <option key={p}>{p}</option>)}
            </select>
          </Field>
          <Field label="Duration (min)"><input type="number" value={modal?.duration_min ?? ''} onChange={(e) => setModal({ ...(modal as object), duration_min: Number(e.target.value) })} /></Field>
        </div>
        <label className="check-row"><input type="checkbox" checked={modal?.published ?? true} onChange={(e) => setModal({ ...(modal as object), published: e.target.checked })} /><span>Published to members</span></label>
      </Modal>
    </div>
  );
}