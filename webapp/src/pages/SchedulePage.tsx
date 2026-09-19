import { useEffect, useState } from 'react';
import { useAuth } from '../auth';
import { Card, Button, Modal, Field, Spinner, Empty, Badge } from '../components/ui';
import { Icon } from '../components/icons';
import { fetchServices, fetchEvents, saveService, deleteService, saveEvent, deleteEvent, type Service, type ChurchEvent } from '../lib/api';
import { SERVICE_TYPES, EVENT_CATEGORIES, WEEKDAYS } from '../roles';
import { todayISO, formatDate } from '../utils';
import { useToast } from '../components/toast';

export function SchedulePage() {
  const { canEdit } = useAuth();
  const toast = useToast();
  const [tab, setTab] = useState<'services' | 'events'>('services');
  const [services, setServices] = useState<Service[] | null>(null);
  const [events, setEvents] = useState<ChurchEvent[] | null>(null);
  const [svcModal, setSvcModal] = useState<Partial<Service> | 'new' | null>(null);
  const [evtModal, setEvtModal] = useState<Partial<ChurchEvent> | 'new' | null>(null);
  const [busy, setBusy] = useState(false);

  function load() {
    fetchServices().then(setServices).catch(() => setServices([]));
    fetchEvents().then(setEvents).catch(() => setEvents([]));
  }
  useEffect(load, []);

  async function saveSvc() {
    if (!svcModal || svcModal === 'new') return;
    setBusy(true);
    try {
      await saveService({ ...svcModal, name: svcModal.name || '', type: svcModal.type || '', start_time: svcModal.start_time || '', end_time: svcModal.end_time || '', location: svcModal.location || '' });
      toast('Service saved.', 'success'); setSvcModal(null); load();
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
    finally { setBusy(false); }
  }

  async function saveEvt() {
    if (!evtModal || evtModal === 'new') return;
    setBusy(true);
    try {
      await saveEvent({ ...evtModal, title: evtModal.title || '', date: evtModal.date || todayISO() });
      toast('Event saved.', 'success'); setEvtModal(null); load();
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
    finally { setBusy(false); }
  }

  function groupedServices(): { rec: Service[]; once: Service[] } {
    if (!services) return { rec: [], once: [] };
    const rec = services.filter((s) => s.recurring);
    const once = services.filter((s) => !s.recurring).sort((a, b) => (a.date || '').localeCompare(b.date || ''));
    return { rec, once };
  }

  return (
    <div className="stack">
      <Card title="Services & Events">
        <div className="tabs">
          <button className={tab === 'services' ? 'active' : ''} onClick={() => setTab('services')}>Services</button>
          <button className={tab === 'events' ? 'active' : ''} onClick={() => setTab('events')}>Events & activities</button>
        </div>
      </Card>

      {tab === 'services' ? (
        !services ? <Spinner label="Loading services…" />
        : services.length === 0 ? <Card><Empty title="No services yet" sub="Leaders can add the weekly schedule and one-off services." /></Card>
        : (<>
          <Card title="Recurring (weekly)"
            actions={canEdit ? <Button onClick={() => setSvcModal({ recurring: true, weekday: 0 })}><Icon.Plus size={16} /> Add</Button> : undefined}>
            <div className="list">
              {groupedServices().rec.map((s) => (
                <div key={s.id} className="list-row">
                  <div className="list-main">
                    <div className="list-title">{s.name}</div>
                    <div className="muted small">{s.type} · every {WEEKDAYS[s.weekday ?? 0]} · {s.start_time}{s.end_time ? `–${s.end_time}` : ''}{s.location ? ` · ${s.location}` : ''}</div>
                  </div>
                  <Badge tone="success">Weekly</Badge>
                  {canEdit && <RowActions onEdit={() => setSvcModal(s)} onDelete={() => confirm('Delete this service?') && deleteService(s.id).then(() => { toast('Service deleted.', 'success'); load(); }).catch((e) => toast(String(e), 'error'))} />}
                </div>
              ))}
            </div>
          </Card>

          <Card title="One-off services"
            actions={canEdit ? <Button onClick={() => setSvcModal({ recurring: false, date: todayISO() })}><Icon.Plus size={16} /> Add</Button> : undefined}>
            {groupedServices().once.length === 0
              ? <Empty title="No one-off services" />
              : <div className="list">
                  {groupedServices().once.map((s) => (
                    <div key={s.id} className="list-row">
                      <div className="list-main">
                        <div className="list-title">{s.name}</div>
                        <div className="muted small">{formatDate(s.date || '')} · {s.start_time}{s.end_time ? `–${s.end_time}` : ''}{s.location ? ` · ${s.location}` : ''}</div>
                      </div>
                      {canEdit && <RowActions onEdit={() => setSvcModal(s)} onDelete={() => confirm('Delete this service?') && deleteService(s.id).then(() => { toast('Service deleted.', 'success'); load(); }).catch((e) => toast(String(e), 'error'))} />}
                    </div>
                  ))}
                </div>}
          </Card>
        </>)
      ) : (
        !events ? <Spinner label="Loading events…" />
        : events.length === 0 ? <Card><Empty title="No events yet" sub="Leaders can add events and activities." /></Card>
        : <Card title="Upcoming & past events"
            actions={canEdit ? <Button onClick={() => setEvtModal({ date: todayISO() })}><Icon.Plus size={16} /> Add event</Button> : undefined}>
            <div className="list">
              {events.map((e) => (
                <div key={e.id} className="list-row">
                  <div className="list-date">
                    <div className="list-day">{new Date(e.date).getDate()}</div>
                    <div className="muted small">{new Date(e.date + 'T00:00:00').toLocaleString(undefined, { month: 'short' })}</div>
                  </div>
                  <div className="list-main">
                    <div className="list-title">{e.title}</div>
                    <div className="muted small">{e.category} · {e.start_time}{e.end_time ? `–${e.end_time}` : ''}{e.location ? ` · ${e.location}` : ''}</div>
                    {e.description && <div className="muted small">{e.description}</div>}
                  </div>
                  {canEdit && <RowActions onEdit={() => setEvtModal(e)} onDelete={() => confirm('Delete this event?') && deleteEvent(e.id).then(() => { toast('Event deleted.', 'success'); load(); }).catch((x) => toast(String(x), 'error'))} />}
                </div>
              ))}
            </div>
          </Card>
      )}

      <ServiceModal editing={svcModal} onClose={() => setSvcModal(null)} onSave={saveSvc} busy={busy} />
      <EventModal editing={evtModal} onClose={() => setEvtModal(null)} onSave={saveEvt} busy={busy} />
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

function ServiceModal({ editing, onClose, onSave, busy }: {
  editing: Partial<Service> | 'new' | null; onClose: () => void; onSave: () => void; busy: boolean;
}) {
  const [d, setD] = useState<Partial<Service>>({});
  useEffect(() => { setD(editing && editing !== 'new' ? { ...editing } : { recurring: false, date: todayISO() }); }, [editing]);
  return (
    <Modal open={!!editing} title={editing === 'new' ? 'Add service' : 'Edit service'} onClose={onClose}
      footer={<><Button variant="ghost" onClick={onClose}>Cancel</Button><Button disabled={busy || !d.name} onClick={onSave}>{busy ? 'Saving…' : 'Save'}</Button></>}>
      <Field label="Name"><input value={d.name || ''} onChange={(e) => setD({ ...d, name: e.target.value })} /></Field>
      <Field label="Type">
        <select value={d.type || SERVICE_TYPES[0]} onChange={(e) => setD({ ...d, type: e.target.value })}>
          {SERVICE_TYPES.map((t) => <option key={t}>{t}</option>)}
        </select>
      </Field>
      <Field label="Schedule">
        <select value={d.recurring ? 'recurring' : 'once'}
          onChange={(e) => setD({ ...d, recurring: e.target.value === 'recurring' })}>
          <option value="recurring">Every week on a fixed day</option>
          <option value="once">One specific date</option>
        </select>
      </Field>
      {d.recurring ? (
        <Field label="Weekday">
          <select value={d.weekday ?? 0} onChange={(e) => setD({ ...d, weekday: Number(e.target.value) })}>
            {WEEKDAYS.map((w, i) => <option key={w} value={i}>{w}</option>)}
          </select>
        </Field>
      ) : (
        <Field label="Date"><input type="date" value={d.date || todayISO()} onChange={(e) => setD({ ...d, date: e.target.value })} /></Field>
      )}
      <div className="row-2">
        <Field label="Start (HH:MM)"><input value={d.start_time || ''} onChange={(e) => setD({ ...d, start_time: e.target.value })} placeholder="10:00" /></Field>
        <Field label="End (HH:MM)"><input value={d.end_time || ''} onChange={(e) => setD({ ...d, end_time: e.target.value })} placeholder="12:30" /></Field>
      </div>
      <Field label="Location"><input value={d.location || ''} onChange={(e) => setD({ ...d, location: e.target.value })} /></Field>
    </Modal>
  );
}

function EventModal({ editing, onClose, onSave, busy }: {
  editing: Partial<ChurchEvent> | 'new' | null; onClose: () => void; onSave: () => void; busy: boolean;
}) {
  const [d, setD] = useState<Partial<ChurchEvent>>({});
  useEffect(() => { setD(editing && editing !== 'new' ? { ...editing } : { date: todayISO() }); }, [editing]);
  return (
    <Modal open={!!editing} title={editing === 'new' ? 'Add event' : 'Edit event'} onClose={onClose}
      footer={<><Button variant="ghost" onClick={onClose}>Cancel</Button><Button disabled={busy || !d.title || !d.date} onClick={onSave}>{busy ? 'Saving…' : 'Save'}</Button></>}>
      <Field label="Title"><input value={d.title || ''} onChange={(e) => setD({ ...d, title: e.target.value })} /></Field>
      <Field label="Category">
        <select value={d.category || EVENT_CATEGORIES[0]} onChange={(e) => setD({ ...d, category: e.target.value })}>
          {EVENT_CATEGORIES.map((c) => <option key={c}>{c}</option>)}
        </select>
      </Field>
      <Field label="Date"><input type="date" value={d.date || todayISO()} onChange={(e) => setD({ ...d, date: e.target.value })} /></Field>
      <div className="row-2">
        <Field label="Start"><input value={d.start_time || ''} onChange={(e) => setD({ ...d, start_time: e.target.value })} placeholder="10:00" /></Field>
        <Field label="End"><input value={d.end_time || ''} onChange={(e) => setD({ ...d, end_time: e.target.value })} placeholder="12:00" /></Field>
      </div>
      <Field label="Location"><input value={d.location || ''} onChange={(e) => setD({ ...d, location: e.target.value })} /></Field>
      <Field label="Description"><textarea value={d.description || ''} onChange={(e) => setD({ ...d, description: e.target.value })} rows={3} /></Field>
    </Modal>
  );
}