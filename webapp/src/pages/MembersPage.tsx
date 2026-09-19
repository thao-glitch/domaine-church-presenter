import { useEffect, useMemo, useState } from 'react';
import { useAuth } from '../auth';
import { Card, Button, Modal, Field, Spinner, Empty, Avatar, RoleBadge, Badge } from '../components/ui';
import { Icon } from '../components/icons';
import { fetchMembers, saveMember, deleteMember, type Member } from '../lib/api';
import { ROLES, roleIndex } from '../roles';
import { roleDef } from '../roles';
import { useToast } from '../components/toast';

export function MembersPage() {
  const { canEdit } = useAuth();
  const toast = useToast();
  const [members, setMembers] = useState<Member[] | null>(null);
  const [q, setQ] = useState('');
  const [editing, setEditing] = useState<Partial<Member> | 'new' | null>(null);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);

  async function load() {
    try {
      setMembers(await fetchMembers());
      setError(null);
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e));
      setMembers([]);
    }
  }

  useEffect(() => { load(); }, []);

  const sorted = useMemo(() => {
    const byRole: Record<string, Member[]> = {};
    for (const m of members || []) {
      const k = String(m.role || 'Member');
      (byRole[k] = byRole[k] || []).push(m);
    }
    return ROLES.map((r) => ({ role: r.name, items: byRole[r.name] || [] }))
      .filter((g) => g.items.length > 0);
  }, [members]);

  const filtered = q.trim()
    ? sorted.map((g) => ({
        role: g.role,
        items: g.items.filter((m) =>
          (m.full_name || '').toLowerCase().includes(q.toLowerCase()) ||
          (m.email || '').toLowerCase().includes(q.toLowerCase()))
      })).filter((g) => g.items.length > 0)
    : sorted;

  async function onSave() {
    if (!editing || editing === 'new') return;
    setBusy(true);
    try {
      await saveMember({ id: (editing as Member)?.id, full_name: editing.full_name || '', phone: editing.phone || '', email: editing.email || '', role: editing.role || 'Member' });
      toast('Member saved.', 'success');
      setEditing(null);
      load();
    } catch (e) {
      toast(e instanceof Error ? e.message : String(e), 'error');
    } finally {
      setBusy(false);
    }
  }

  async function onDelete(m: Member) {
    if (!confirm(`Remove ${m.full_name || m.email} from the directory?`)) return;
    try {
      await deleteMember(m.id);
      toast('Member removed.', 'success');
      load();
    } catch (e) {
      toast(e instanceof Error ? e.message : String(e), 'error');
    }
  }

  return (
    <div className="stack">
      <Card title="Members directory"
        actions={canEdit ? <Button onClick={() => setEditing('new')}><Icon.Plus size={16} /> Add member</Button> : undefined}>
        <div className="toolbar">
          <div className="search">
            <Icon.Search size={16} />
            <input placeholder="Search name or email…" value={q} onChange={(e) => setQ(e.target.value)} />
          </div>
          {error && <span className="form-error">{error}</span>}
        </div>
      </Card>

      {!members ? <Spinner label="Loading members…" />
       : sorted.length === 0 ? <Card><Empty title="No members yet" sub="Leaders can add members, or new users appear after they create accounts." /></Card>
       : filtered.length === 0 ? <Card><Empty title="No matches" sub={`Nothing for "${q}".`} /></Card>
       : filtered.map((g) => (
          <Card key={g.role} title={<span><Badge tone={roleIndex(g.role) <= 4 ? 'leader' : roleDef(g.role).tier === 'editor' ? 'editor' : 'default'}>{g.role}</Badge> <span className="muted small">· {g.items.length}</span></span>}>
            <div className="member-list">
              {g.items.map((m) => (
                <div key={m.id} className="member-row">
                  <Avatar name={m.full_name || m.email || '?'} />
                  <div className="member-main">
                    <div className="member-name">{m.full_name || m.email}</div>
                    <div className="muted small">{[m.phone, m.email].filter(Boolean).join(' · ') || '—'}</div>
                  </div>
                  <RoleBadge role={m.role} />
                  {canEdit && (
                    <div className="row-actions">
                      <button className="icon-btn" onClick={() => setEditing(m)} title="Edit"><Icon.Edit size={16} /></button>
                      <button className="icon-btn danger" onClick={() => onDelete(m)} title="Remove"><Icon.Trash size={16} /></button>
                    </div>
                  )}
                </div>
              ))}
            </div>
          </Card>
        ))}

      <MemberModal editing={editing} onClose={() => setEditing(null)} onSave={onSave} busy={busy} />
    </div>
  );
}

function MemberModal({ editing, onClose, onSave, busy }: {
  editing: Partial<Member> | 'new' | null; onClose: () => void; onSave: () => void; busy: boolean;
}) {
  const [draft, setDraft] = useState<Partial<Member>>({});
  useEffect(() => {
    setDraft(editing && editing !== 'new' ? { ...editing } : { role: 'Member' });
  }, [editing]);
  return (
    <Modal open={!!editing} title={editing === 'new' ? 'Add member' : 'Edit member'} onClose={onClose}
      footer={<>
        <Button variant="ghost" onClick={onClose}>Cancel</Button>
        <Button disabled={busy || !draft.full_name} onClick={onSave}>{busy ? 'Saving…' : 'Save'}</Button>
      </>}>
      <Field label="Full name">
        <input value={draft.full_name || ''} onChange={(e) => setDraft({ ...draft, full_name: e.target.value })} />
      </Field>
      <Field label="Role">
        <select value={draft.role || 'Member'} onChange={(e) => setDraft({ ...draft, role: e.target.value })}>
          {ROLES.map((r) => <option key={r.name} value={r.name}>{r.name}</option>)}
        </select>
      </Field>
      <Field label="Phone">
        <input value={draft.phone || ''} onChange={(e) => setDraft({ ...draft, phone: e.target.value })} />
      </Field>
      <Field label="Email">
        <input value={draft.email || ''} onChange={(e) => setDraft({ ...draft, email: e.target.value })} />
      </Field>
    </Modal>
  );
}