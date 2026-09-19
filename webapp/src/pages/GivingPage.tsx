import { useEffect, useMemo, useState } from 'react';
import { useAuth } from '../auth';
import { Card, Button, Modal, Field, Badge, Spinner, Empty } from '../components/ui';
import { Icon } from '../components/icons';
import { fetchGiving, saveGiving, deleteGiving, type GivingRow } from '../lib/churchlife';
import { useToast } from '../components/toast';
import { todayISO, formatDate } from '../utils';

const PURPOSES = ['tithe', 'offering', 'pledge', 'project', 'missions', 'welfare', 'thanksgiving'];
const METHODS = ['cash', 'mobile', 'card', 'bank', 'cheque'];

export function GivingPage() {
  const { canEdit, user } = useAuth();
  const toast = useToast();
  const [rows, setRows] = useState<GivingRow[] | null>(null);
  const [modal, setModal] = useState<Partial<GivingRow> | null>(null);
  const [busy, setBusy] = useState(false);

  function load() {
    fetchGiving().then(setRows).catch(() => setRows([]));
  }
  useEffect(load, []); // eslint-disable-line react-hooks/exhaustive-deps

  const thisMonth = todayISO().slice(0, 7);
  const totals = useMemo(() => {
    const all = rows || [];
    const month = all.filter((r) => (r.given_on || '').startsWith(thisMonth));
    const sum = (arr: GivingRow[]) => arr.reduce((s, r) => s + Number(r.amount || 0), 0);
    return { all: sum(all), month: sum(month), count: all.length };
  }, [rows, thisMonth]);

  async function save() {
    if (!modal) return;
    setBusy(true);
    try {
      await saveGiving({ ...modal, amount: Number(modal.amount || 0), recorded_by: user?.email || null });
      toast('Giving recorded.', 'success'); setModal(null); load();
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
    finally { setBusy(false); }
  }

  return (
    <div className="stack">
      <Card title="Giving & offerings"
        actions={canEdit ? <Button onClick={() => setModal({ given_on: todayISO(), purpose: 'tithe', method: 'cash', currency: 'USD' })}><Icon.Plus size={16} /> Record giving</Button> : undefined}>
        <div className="stat-row">
          <div className="stat"><div className="stat-value">{totals.month.toLocaleString()}</div><div className="stat-label">this month</div></div>
          <div className="stat"><div className="stat-value">{totals.all.toLocaleString()}</div><div className="stat-label">all time</div></div>
          <div className="stat"><div className="stat-value">{totals.count}</div><div className="stat-label">records</div></div>
        </div>
        {!canEdit && <p className="muted small">You can see your own giving records here.</p>}
      </Card>

      <Card title="Records">
        {!rows ? <Spinner /> : rows.length === 0 ? <Empty title="No giving recorded yet" /> : (
          <div className="list">
            {rows.map((r) => (
              <div key={r.id} className="list-row">
                <div className="list-main">
                  <div className="list-title">{r.giver_name || r.giver_email || 'Anonymous'} <Badge tone="success">{r.currency} {Number(r.amount).toLocaleString()}</Badge></div>
                  <div className="muted small">{r.purpose} · {r.method} · {formatDate(r.given_on)}{r.reference ? ` · ${r.reference}` : ''}</div>
                </div>
                {canEdit && (
                  <div className="row-actions">
                    <button className="icon-btn" onClick={() => setModal(r)}><Icon.Edit size={16} /></button>
                    <button className="icon-btn danger" onClick={() => confirm('Delete this record?') && deleteGiving(r.id).then(() => { toast('Deleted.', 'success'); load(); })}><Icon.Trash size={16} /></button>
                  </div>
                )}
              </div>
            ))}
          </div>
        )}
      </Card>

      <Modal open={!!modal} title={modal?.id ? 'Edit giving' : 'Record giving'} onClose={() => setModal(null)}
        footer={<><Button variant="ghost" onClick={() => setModal(null)}>Cancel</Button><Button disabled={busy || !modal?.amount} onClick={save}>Save</Button></>}>
        <div className="row-2">
          <Field label="Giver name"><input value={modal?.giver_name || ''} onChange={(e) => setModal({ ...(modal as object), giver_name: e.target.value })} /></Field>
          <Field label="Giver email"><input value={modal?.giver_email || ''} onChange={(e) => setModal({ ...(modal as object), giver_email: e.target.value })} /></Field>
        </div>
        <div className="row-2">
          <Field label="Amount"><input type="number" value={modal?.amount ?? ''} onChange={(e) => setModal({ ...(modal as object), amount: Number(e.target.value) })} /></Field>
          <Field label="Currency"><input value={modal?.currency || 'USD'} onChange={(e) => setModal({ ...(modal as object), currency: e.target.value })} /></Field>
        </div>
        <div className="row-2">
          <Field label="Purpose">
            <select value={modal?.purpose || 'tithe'} onChange={(e) => setModal({ ...(modal as object), purpose: e.target.value })}>
              {PURPOSES.map((p) => <option key={p}>{p}</option>)}
            </select>
          </Field>
          <Field label="Method">
            <select value={modal?.method || 'cash'} onChange={(e) => setModal({ ...(modal as object), method: e.target.value })}>
              {METHODS.map((m) => <option key={m}>{m}</option>)}
            </select>
          </Field>
        </div>
        <div className="row-2">
          <Field label="Date"><input type="date" value={(modal?.given_on || todayISO()).slice(0, 10)} onChange={(e) => setModal({ ...(modal as object), given_on: e.target.value })} /></Field>
          <Field label="Reference"><input value={modal?.reference || ''} onChange={(e) => setModal({ ...(modal as object), reference: e.target.value })} /></Field>
        </div>
      </Modal>
    </div>
  );
}