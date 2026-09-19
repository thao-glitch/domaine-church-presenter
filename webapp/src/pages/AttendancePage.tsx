import { useEffect, useState } from 'react';
import { useAuth } from '../auth';
import { Card, Button, Modal, Field, Badge, Spinner, Empty } from '../components/ui';
import { Icon } from '../components/icons';
import {
  fetchAttendance, addAttendance, deleteAttendance,
  fetchFollowUps, saveFollowUp, deleteFollowUp,
  type AttendanceRow, type FollowUp
} from '../lib/churchlife';
import { useToast } from '../components/toast';
import { todayISO, formatDate } from '../utils';

const STAGES = ['new', 'contacted', 'visited', 'joined'];
const STAGE_TONE: Record<string, 'warn' | 'editor' | 'default' | 'success'> = {
  new: 'warn', contacted: 'editor', visited: 'default', joined: 'success'
};

export function AttendancePage() {
  const { canEdit } = useAuth();
  const toast = useToast();
  const [date, setDate] = useState(todayISO());
  const [rows, setRows] = useState<AttendanceRow[] | null>(null);
  const [name, setName] = useState('');
  const [firstTimer, setFirstTimer] = useState(false);
  const [follows, setFollows] = useState<FollowUp[] | null>(null);
  const [fuModal, setFuModal] = useState<Partial<FollowUp> | null>(null);
  const [busy, setBusy] = useState(false);

  function load() {
    fetchAttendance(date).then(setRows).catch(() => setRows([]));
    fetchFollowUps().then(setFollows).catch(() => setFollows([]));
  }
  useEffect(load, [date]); // eslint-disable-line react-hooks/exhaustive-deps

  if (!canEdit) return <Empty title="Leaders only" sub="Attendance and follow-up are managed by church leaders." />;

  async function checkIn() {
    if (!name.trim()) return;
    try {
      await addAttendance(name.trim(), date, firstTimer ? 'first_timer' : 'present');
      if (firstTimer) await saveFollowUp({ person_name: name.trim(), is_first_timer: true, stage: 'new' });
      setName(''); setFirstTimer(false);
      toast('Checked in.', 'success'); load();
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
  }

  async function saveFu() {
    if (!fuModal || !fuModal.person_name?.trim()) return;
    setBusy(true);
    try {
      await saveFollowUp({ ...fuModal, person_name: fuModal.person_name.trim() });
      toast('Follow-up saved.', 'success'); setFuModal(null); load();
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
    finally { setBusy(false); }
  }

  const present = (rows || []).filter((r) => r.status !== 'first_timer').length;
  const visitors = (rows || []).filter((r) => r.status === 'first_timer').length;

  return (
    <div className="stack">
      <Card title="Attendance"
        actions={<input type="date" value={date} onChange={(e) => setDate(e.target.value)} />}>
        <div className="muted small">{present} present · {visitors} first-timer{visitors === 1 ? '' : 's'} · {formatDate(date)}</div>
        <div className="spacer-s" />
        <div className="checkin-row">
          <input placeholder="Name to check in…" value={name} onChange={(e) => setName(e.target.value)}
            onKeyDown={(e) => e.key === 'Enter' && checkIn()} />
          <label className="check-row"><input type="checkbox" checked={firstTimer} onChange={(e) => setFirstTimer(e.target.checked)} /><span>First timer</span></label>
          <Button onClick={checkIn} disabled={!name.trim()}>Check in</Button>
        </div>
      </Card>

      <Card title={`Checked in — ${formatDate(date)}`}>
        {!rows ? <Spinner /> : rows.length === 0 ? <Empty title="No one checked in yet" sub="Add names as people arrive." /> : (
          <div className="list">
            {rows.map((r) => (
              <div key={r.id} className="list-row">
                <div className="list-main">
                  <div className="list-title">{r.person_name} {r.status === 'first_timer' && <Badge tone="warn">first timer</Badge>}</div>
                  <div className="muted small">checked in {new Date(r.checked_in_at).toLocaleTimeString()}</div>
                </div>
                <button className="icon-btn danger" onClick={() => deleteAttendance(r.id).then(() => { toast('Removed.', 'success'); load(); })}><Icon.Trash size={16} /></button>
              </div>
            ))}
          </div>
        )}
      </Card>

      <Card title="Follow-up pipeline"
        actions={<Button onClick={() => setFuModal({ is_first_timer: true, stage: 'new' })}><Icon.Plus size={16} /> Add person</Button>}>
        {!follows ? <Spinner /> : follows.length === 0 ? <Empty title="No follow-ups" sub="First-timers are added here automatically." /> : (
          <div className="list">
            {follows.map((f) => (
              <div key={f.id} className="list-row">
                <div className="list-main">
                  <div className="list-title">{f.person_name} <Badge tone={STAGE_TONE[f.stage]}>{f.stage}</Badge></div>
                  <div className="muted small">{f.phone || f.email || 'no contact'}{f.assigned_to ? ` · assigned ${f.assigned_to}` : ''}</div>
                  {f.notes && <div className="muted small">{f.notes}</div>}
                </div>
                <select className="role-select" value={f.stage} onChange={(e) => saveFollowUp({ ...f, stage: e.target.value }).then(() => { toast('Stage updated.', 'success'); load(); })}>
                  {STAGES.map((s) => <option key={s} value={s}>{s}</option>)}
                </select>
                <button className="icon-btn" onClick={() => setFuModal(f)}><Icon.Edit size={16} /></button>
                <button className="icon-btn danger" onClick={() => confirm('Delete this follow-up?') && deleteFollowUp(f.id).then(() => { toast('Deleted.', 'success'); load(); })}><Icon.Trash size={16} /></button>
              </div>
            ))}
          </div>
        )}
      </Card>

      <Modal open={!!fuModal} title={fuModal?.id ? 'Edit follow-up' : 'Add follow-up'} onClose={() => setFuModal(null)}
        footer={<><Button variant="ghost" onClick={() => setFuModal(null)}>Cancel</Button><Button disabled={busy || !fuModal?.person_name?.trim()} onClick={saveFu}>Save</Button></>}>
        <Field label="Name"><input value={fuModal?.person_name || ''} onChange={(e) => setFuModal({ ...(fuModal as object), person_name: e.target.value })} /></Field>
        <div className="row-2">
          <Field label="Phone"><input value={fuModal?.phone || ''} onChange={(e) => setFuModal({ ...(fuModal as object), phone: e.target.value })} /></Field>
          <Field label="Email"><input value={fuModal?.email || ''} onChange={(e) => setFuModal({ ...(fuModal as object), email: e.target.value })} /></Field>
        </div>
        <Field label="Stage">
          <select value={fuModal?.stage || 'new'} onChange={(e) => setFuModal({ ...(fuModal as object), stage: e.target.value })}>
            {STAGES.map((s) => <option key={s} value={s}>{s}</option>)}
          </select>
        </Field>
        <Field label="Assigned to"><input value={fuModal?.assigned_to || ''} onChange={(e) => setFuModal({ ...(fuModal as object), assigned_to: e.target.value })} /></Field>
        <Field label="Notes"><textarea rows={3} value={fuModal?.notes || ''} onChange={(e) => setFuModal({ ...(fuModal as object), notes: e.target.value })} /></Field>
      </Modal>
    </div>
  );
}