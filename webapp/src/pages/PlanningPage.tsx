import { useEffect, useState } from 'react';
import { useAuth } from '../auth';
import { Card, Button, Modal, Field, Badge, Spinner, Empty } from '../components/ui';
import { Icon } from '../components/icons';
import {
  fetchPlans, savePlan, deletePlan, fetchPlanItems, savePlanItem, deletePlanItem,
  fetchSlots, saveSlot, deleteSlot, fetchAssignments, saveAssignment, deleteAssignment,
  type ServicePlan, type PlanItem, type VolunteerSlot, type VolunteerAssignment
} from '../lib/churchlife';
import { useToast } from '../components/toast';
import { todayISO, formatDate } from '../utils';

const ITEM_TYPES = ['Welcome', 'Opening Prayer', 'Praise & Worship', 'Scripture Reading', 'Sermon', 'Offering', 'Announcements', 'Communion', 'Special Number', 'Benediction', 'Custom'];
const VOL_ROLES = ['Worship', 'Sound', 'Media', 'Ushers', 'Greeters', 'Children', 'Prayer', 'Other'];

export function PlanningPage() {
  const { canEdit } = useAuth();
  const toast = useToast();
  const [tab, setTab] = useState<'plan' | 'roster'>('plan');

  const [plans, setPlans] = useState<ServicePlan[] | null>(null);
  const [active, setActive] = useState<ServicePlan | null>(null);
  const [items, setItems] = useState<PlanItem[]>([]);
  const [planModal, setPlanModal] = useState<Partial<ServicePlan> | 'new' | null>(null);
  const [itemModal, setItemModal] = useState<Partial<PlanItem> | 'new' | null>(null);

  const [slots, setSlots] = useState<VolunteerSlot[] | null>(null);
  const [assignments, setAssignments] = useState<VolunteerAssignment[]>([]);
  const [slotModal, setSlotModal] = useState<Partial<VolunteerSlot> | 'new' | null>(null);
  const [assignFor, setAssignFor] = useState<VolunteerSlot | null>(null);

  const [busy, setBusy] = useState(false);

  function load() {
    fetchPlans().then((p) => { setPlans(p); if (!active && p[0]) selectPlan(p[0]); }).catch(() => setPlans([]));
    fetchSlots().then(setSlots).catch(() => setSlots([]));
    fetchAssignments().then(setAssignments).catch(() => setAssignments([]));
  }
  useEffect(load, []); // eslint-disable-line react-hooks/exhaustive-deps

  function selectPlan(p: ServicePlan) {
    setActive(p);
    fetchPlanItems(p.id).then(setItems).catch(() => setItems([]));
  }

  async function savePlanModal() {
    if (!planModal || planModal === 'new') return;
    setBusy(true);
    try {
      await savePlan({ ...planModal, title: planModal.title || 'Service' });
      toast('Plan saved.', 'success'); setPlanModal(null); load();
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
    finally { setBusy(false); }
  }

  async function saveItemModal() {
    if (!itemModal || itemModal === 'new' || !active) return;
    setBusy(true);
    try {
      await savePlanItem({ ...itemModal, plan_id: active.id, title: itemModal.title || 'Item', position: itemModal.position ?? items.length });
      toast('Item saved.', 'success'); setItemModal(null); fetchPlanItems(active.id).then(setItems);
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
    finally { setBusy(false); }
  }

  async function saveSlotModal() {
    if (!slotModal || slotModal === 'new') return;
    setBusy(true);
    try {
      await saveSlot({ ...slotModal, role: slotModal.role || 'Other' });
      toast('Slot saved.', 'success'); setSlotModal(null); load();
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
    finally { setBusy(false); }
  }

  async function assign(slot: VolunteerSlot, name: string, status: string) {
    try {
      await saveAssignment({ slot_id: slot.id, name, status });
      toast(`${name} ${status}.`, 'success');
      fetchAssignments().then(setAssignments);
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
  }

  return (
    <div className="stack">
      <Card title="Service planning">
        <div className="tabs">
          <button className={tab === 'plan' ? 'active' : ''} onClick={() => setTab('plan')}>Order of service</button>
          <button className={tab === 'roster' ? 'active' : ''} onClick={() => setTab('roster')}>Volunteer roster</button>
        </div>
      </Card>

      {tab === 'plan' && (
        <>
          <Card title="Plans"
            actions={canEdit ? <Button onClick={() => setPlanModal({ plan_date: todayISO(), title: 'Sunday Service' })}><Icon.Plus size={16} /> New plan</Button> : undefined}>
            {!plans ? <Spinner /> : plans.length === 0 ? <Empty title="No plans yet" sub="Create an order of service for an upcoming meeting." /> : (
              <div className="list">
                {plans.map((p) => (
                  <div key={p.id} className={`list-row ${active?.id === p.id ? 'active' : ''}`} onClick={() => selectPlan(p)} style={{ cursor: 'pointer' }}>
                    <div className="list-main">
                      <div className="list-title">{p.title}</div>
                      <div className="muted small">{formatDate(p.plan_date)}{p.theme ? ` · ${p.theme}` : ''}</div>
                    </div>
                    {canEdit && (
                      <div className="row-actions" onClick={(e) => e.stopPropagation()}>
                        <button className="icon-btn" onClick={() => setPlanModal(p)}><Icon.Edit size={16} /></button>
                        <button className="icon-btn danger" onClick={() => confirm('Delete this plan?') && deletePlan(p.id).then(() => { toast('Deleted.', 'success'); setActive(null); load(); })}><Icon.Trash size={16} /></button>
                      </div>
                    )}
                  </div>
                ))}
              </div>
            )}
          </Card>

          {active && (
            <Card title={`Order of service — ${active.title}`}
              actions={canEdit ? <Button variant="soft" onClick={() => setItemModal({ item_type: 'Custom', position: items.length })}><Icon.Plus size={16} /> Add item</Button> : undefined}>
              {items.length === 0 ? <Empty title="No items yet" sub="Add welcome, worship, sermon, offering…" /> : (
                <div className="list">
                  {items.map((it, i) => (
                    <div key={it.id} className="list-row">
                      <div className="list-idx">{i + 1}</div>
                      <div className="list-main">
                        <div className="list-title">{it.title} {it.item_type !== 'Custom' && <Badge tone="editor">{it.item_type}</Badge>}</div>
                        <div className="muted small">{it.person || 'unassigned'}{it.duration_min ? ` · ${it.duration_min} min` : ''}{it.notes ? ` · ${it.notes}` : ''}</div>
                      </div>
                      {canEdit && (
                        <div className="row-actions">
                          <button className="icon-btn" onClick={() => setItemModal(it)}><Icon.Edit size={16} /></button>
                          <button className="icon-btn danger" onClick={() => deletePlanItem(it.id).then(() => fetchPlanItems(active.id).then(setItems))}><Icon.Trash size={16} /></button>
                        </div>
                      )}
                    </div>
                  ))}
                </div>
              )}
            </Card>
          )}
        </>
      )}

      {tab === 'roster' && (
        <Card title="Volunteer roster"
          actions={canEdit ? <Button onClick={() => setSlotModal({ role: 'Worship', needed: 1, service_date: todayISO() })}><Icon.Plus size={16} /> Add role</Button> : undefined}>
          {!slots ? <Spinner /> : slots.length === 0 ? <Empty title="No roles yet" sub="Add serving roles and invite people." /> : (
            <div className="list">
              {slots.map((s) => {
                const list = assignments.filter((a) => a.slot_id === s.id);
                const filled = list.filter((a) => a.status === 'confirmed').length;
                return (
                  <div key={s.id} className="list-block">
                    <div className="list-row">
                      <div className="list-main">
                        <div className="list-title">{s.title || s.role} <Badge tone={filled >= s.needed ? 'success' : 'warn'}>{filled}/{s.needed} confirmed</Badge></div>
                        <div className="muted small">{s.role}{s.service_date ? ` · ${formatDate(s.service_date)}` : ''}</div>
                      </div>
                      {canEdit && (
                        <div className="row-actions">
                          <Button variant="soft" onClick={() => setAssignFor(s)}>Assign</Button>
                          <button className="icon-btn" onClick={() => setSlotModal(s)}><Icon.Edit size={16} /></button>
                          <button className="icon-btn danger" onClick={() => confirm('Delete this role?') && deleteSlot(s.id).then(() => { toast('Deleted.', 'success'); load(); })}><Icon.Trash size={16} /></button>
                        </div>
                      )}
                    </div>
                    {list.length > 0 && (
                      <div className="assignment-row">
                        {list.map((a) => (
                          <span key={a.id} className={`chip ${a.status === 'confirmed' ? 'on' : ''}`}>
                            {a.name || 'unassigned'} · {a.status}
                            {canEdit && <button className="chip-x" onClick={() => deleteAssignment(a.id).then(() => fetchAssignments().then(setAssignments))}>×</button>}
                          </span>
                        ))}
                      </div>
                    )}
                  </div>
                );
              })}
            </div>
          )}
        </Card>
      )}

      <Modal open={!!planModal} title={planModal === 'new' ? 'New plan' : 'Edit plan'} onClose={() => setPlanModal(null)}
        footer={<><Button variant="ghost" onClick={() => setPlanModal(null)}>Cancel</Button><Button disabled={busy} onClick={savePlanModal}>Save</Button></>}>
        <Field label="Title"><input value={(planModal && planModal !== 'new' && planModal.title) || ''} onChange={(e) => setPlanModal({ ...(planModal as object), title: e.target.value })} /></Field>
        <Field label="Date"><input type="date" value={(planModal && planModal !== 'new' && planModal.plan_date) || todayISO()} onChange={(e) => setPlanModal({ ...(planModal as object), plan_date: e.target.value })} /></Field>
        <Field label="Theme"><input value={(planModal && planModal !== 'new' && planModal.theme) || ''} onChange={(e) => setPlanModal({ ...(planModal as object), theme: e.target.value })} /></Field>
        <Field label="Notes"><textarea rows={3} value={(planModal && planModal !== 'new' && planModal.notes) || ''} onChange={(e) => setPlanModal({ ...(planModal as object), notes: e.target.value })} /></Field>
      </Modal>

      <Modal open={!!itemModal} title={itemModal === 'new' ? 'Add item' : 'Edit item'} onClose={() => setItemModal(null)}
        footer={<><Button variant="ghost" onClick={() => setItemModal(null)}>Cancel</Button><Button disabled={busy} onClick={saveItemModal}>Save</Button></>}>
        <Field label="Type">
          <select value={(itemModal && itemModal !== 'new' && itemModal.item_type) || 'Custom'} onChange={(e) => setItemModal({ ...(itemModal as object), item_type: e.target.value })}>
            {ITEM_TYPES.map((t) => <option key={t}>{t}</option>)}
          </select>
        </Field>
        <Field label="Title"><input value={(itemModal && itemModal !== 'new' && itemModal.title) || ''} onChange={(e) => setItemModal({ ...(itemModal as object), title: e.target.value })} /></Field>
        <Field label="Led by"><input value={(itemModal && itemModal !== 'new' && itemModal.person) || ''} onChange={(e) => setItemModal({ ...(itemModal as object), person: e.target.value })} /></Field>
        <Field label="Duration (min)"><input type="number" value={(itemModal && itemModal !== 'new' && itemModal.duration_min) || ''} onChange={(e) => setItemModal({ ...(itemModal as object), duration_min: Number(e.target.value) })} /></Field>
        <Field label="Notes"><input value={(itemModal && itemModal !== 'new' && itemModal.notes) || ''} onChange={(e) => setItemModal({ ...(itemModal as object), notes: e.target.value })} /></Field>
      </Modal>

      <Modal open={!!slotModal} title={slotModal === 'new' ? 'Add role' : 'Edit role'} onClose={() => setSlotModal(null)}
        footer={<><Button variant="ghost" onClick={() => setSlotModal(null)}>Cancel</Button><Button disabled={busy} onClick={saveSlotModal}>Save</Button></>}>
        <Field label="Role">
          <select value={(slotModal && slotModal !== 'new' && slotModal.role) || 'Other'} onChange={(e) => setSlotModal({ ...(slotModal as object), role: e.target.value })}>
            {VOL_ROLES.map((r) => <option key={r}>{r}</option>)}
          </select>
        </Field>
        <Field label="Label"><input value={(slotModal && slotModal !== 'new' && slotModal.title) || ''} onChange={(e) => setSlotModal({ ...(slotModal as object), title: e.target.value })} placeholder="e.g. Sunday first service" /></Field>
        <Field label="Date"><input type="date" value={(slotModal && slotModal !== 'new' && slotModal.service_date) || todayISO()} onChange={(e) => setSlotModal({ ...(slotModal as object), service_date: e.target.value })} /></Field>
        <Field label="People needed"><input type="number" value={(slotModal && slotModal !== 'new' && slotModal.needed) || 1} onChange={(e) => setSlotModal({ ...(slotModal as object), needed: Number(e.target.value) })} /></Field>
      </Modal>

      <AssignModal slot={assignFor} onClose={() => setAssignFor(null)} onAssign={assign} />
    </div>
  );
}

function AssignModal({ slot, onClose, onAssign }: {
  slot: VolunteerSlot | null; onClose: () => void;
  onAssign: (s: VolunteerSlot, name: string, status: string) => void;
}) {
  const [name, setName] = useState('');
  const [status, setStatus] = useState('invited');
  useEffect(() => { setName(''); setStatus('invited'); }, [slot]);
  return (
    <Modal open={!!slot} title={`Assign — ${slot?.title || slot?.role || ''}`} onClose={onClose}
      footer={<><Button variant="ghost" onClick={onClose}>Cancel</Button>
        <Button disabled={!name.trim()} onClick={() => { if (slot && name.trim()) { onAssign(slot, name.trim(), status); onClose(); } }}>Add</Button></>}>
      <Field label="Name"><input value={name} onChange={(e) => setName(e.target.value)} /></Field>
      <Field label="Status">
        <select value={status} onChange={(e) => setStatus(e.target.value)}>
          <option value="invited">Invited</option>
          <option value="confirmed">Confirmed</option>
          <option value="declined">Declined</option>
        </select>
      </Field>
    </Modal>
  );
}