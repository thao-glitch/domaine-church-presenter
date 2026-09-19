import { useEffect, useRef, useState } from 'react';
import { useAuth } from '../auth';
import { Card, Button, Field, Spinner, Empty } from '../components/ui';
import { Icon } from '../components/icons';
import { fetchFiles, uploadFile, deleteFileRow, fileUrl, type FileRow } from '../lib/api';
import { fmtBytes, timeAgo, fileNameExt } from '../utils';
import { useToast } from '../components/toast';

const CATEGORIES = ['Songs & Hymns', 'Notes & Documents', 'Announcements', 'Photos', 'Recordings', 'Other'];
const catEmoji: Record<string, string> = {
  'Songs & Hymns': '🎵', 'Notes & Documents': '📄', 'Announcements': '📣',
  'Photos': '🖼️', 'Recordings': '🎤', 'Other': '📦'
};
const CAT_FALLBACK = 'Other';

export function MediaPage() {
  const { user, profile, canEdit } = useAuth();
  const toast = useToast();
  const [files, setFiles] = useState<FileRow[] | null>(null);
  const [category, setCategory] = useState(CATEGORIES[0]);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const inputRef = useRef<HTMLInputElement>(null);

  async function load() {
    try { setFiles(await fetchFiles()); setError(null); }
    catch (e) { setError(e instanceof Error ? e.message : String(e)); setFiles([]); }
  }
  useEffect(() => { load(); }, []);

  async function pick(filesList: FileList | null) {
    if (!filesList || filesList.length === 0) return;
    setBusy(true);
    try {
      const email = user?.email || 'unknown';
      const name = profile?.full_name || email.split('@')[0];
      for (const f of Array.from(filesList)) {
        await uploadFile(f, category, email, name);
      }
      toast(`Uploaded ${filesList.length} file${filesList.length > 1 ? 's' : ''}.`, 'success');
      load();
    } catch (e) {
      toast(e instanceof Error ? e.message : String(e), 'error');
    } finally {
      setBusy(false);
      if (inputRef.current) inputRef.current.value = '';
    }
  }

  async function onDelete(f: FileRow) {
    if (!confirm(`Remove "${f.name}"?`)) return;
    try { await deleteFileRow(f); toast('Deleted.', 'success'); load(); }
    catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
  }

  const canDelete = (f: FileRow) => canEdit || f.uploaded_by === user?.email;

  return (
    <div className="stack">
      <Card title="Media & uploads">
        <p className="muted small">Share songs, notes, announcements, photos and recordings with the whole church. Every member can upload.</p>
        <div className="toolbar wrap">
          <Field label="Store as">
            <select value={category} onChange={(e) => setCategory(e.target.value)}>
              {CATEGORIES.map((c) => <option key={c}>{c}</option>)}
            </select>
          </Field>
          <input ref={inputRef} type="file" multiple hidden
            onChange={(e) => pick(e.target.files)} />
          <Button disabled={busy} onClick={() => inputRef.current?.click()}>
            <Icon.Upload size={16} /> {busy ? 'Uploading…' : 'Upload files'}
          </Button>
        </div>
      </Card>

      {!files ? <Spinner label="Loading files…" />
       : files.length === 0 ? <Card><Empty title="Nothing uploaded yet" sub="Upload the first song, note or announcement for the church." /></Card>
       : <Card title={`All files (${files.length})`}>
          <div className="file-grid">
            {files.map((f) => (
              <div key={f.id} className="file-card">
                <div className="file-icon">{catEmoji[f.category] || catEmoji[CAT_FALLBACK]}</div>
                <div className="file-main">
                  <a className="file-name" href={fileUrl(f.path)} target="_blank" rel="noreferrer" title={f.name}>{f.name}</a>
                  <div className="muted small">{f.category} · {fmtBytes(f.size)} · {timeAgo(f.created_at)}</div>
                  <div className="muted small">by {f.uploaded_by_name || f.uploaded_by}</div>
                </div>
                {canDelete(f) && (
                  <button className="icon-btn danger" onClick={() => onDelete(f)} title="Delete"><Icon.Trash size={16} /></button>
                )}
              </div>
            ))}
          </div>
        </Card>}
      {error && <span className="form-error">{error}</span>}
    </div>
  );
}