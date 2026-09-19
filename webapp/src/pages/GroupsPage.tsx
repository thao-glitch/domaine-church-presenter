import { useEffect, useState } from 'react';
import { useAuth } from '../auth';
import { Card, Button, Modal, Field, Badge, Spinner, Empty, Avatar } from '../components/ui';
import { Icon } from '../components/icons';
import {
  fetchGroups, saveGroup, deleteGroup, fetchGroupMembers, addGroupMember, removeGroupMember,
  type Group, type GroupMember
} from '../lib/churchlife';
import { fetchChurchProfiles, type ChurchProfile } from '../lib/api';
import { WEEKDAYS } from '../roles';
import { useToast } from '../components/toast';

export function GroupsPage() {
  const { canEdit } = useAuth();
  const toast = useToast();
  const [groups, setGroups] = useState<Group[] | null>(null);
  const [active, setActive] = useState<Group | null>(null);
  const [members, setMembers] = useState<GroupMember[]>([]);
  const [people, setPeople] = useState<ChurchProfile[]>([]);
  const [groupModal, setGroupModal] = useState<Partial<Group> | null>(null);
  const [addOpen, setAddOpen] = useState(false);
  const [pick, setPick] = useState('');
  const [role, setRole] = useState('member');
  const [busy, setBusy] = useState(false);

  function load() {
    fetchGroups().then((g) => { setGroups(g); if (!active && g[0]) select(g[0]); }).catch(() => setGroups([]));
    fetchChurchProfiles().then(setPeople).catch(() => setPeople([]));
  }
  useEffect(load, []); // eslint-disable-line react-hooks/exhaustive-deps

  function select(g: Group) {
    setActive(g);
    fetchGroupMembers(g.id).then(setMembers).catch(() => setMembers([]));
  }

  async function saveGroupModal() {
    if (!groupModal || !groupModal.name?.trim()) return;
    setBusy(true);
    try {
      await saveGroup({ ...groupModal, name: groupModal.name.trim() });
      toast('Group saved.', 'success'); setGroupModal(null); load();
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
    finally { setBusy(false); }
  }

  async function add() {
    if (!active || !pick) return;
    const p = people.find((x) => x.id === pick);
    if (!p) return;
    try {
      await addGroupMember(active.id, p.id, p.full_name || p.email, role);
      toast('Member added.', 'success'); setAddOpen(false); setPick(''); setRole('member');
      fetchGroupMembers(active.id).then(setMembers);
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
  }

  async function toggleRole(m: GroupMember) {
    const next = m.member_role === 'leader' ? 'member' : 'leader';
    try {
      await removeGroupMember(m.id);
      await addGroupMember(m.group_id, m.profile_id || '', m.name || '', next);
      fetchGroupMembers(m.group_id).then(setMembers);
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
  }

  return (
    <div className="stack">
      <Card title="Groups & ministries"
        actions={canEdit ? <Button onClick={() => setGroupModal({ kind: 'ministry' })}><Icon.Plus size={16} /> New group</Button> : undefined}>
        <p className="muted small">Cell groups, choirs, youth, ushering — each with its own leader and members.</p>
      </Card>

      <div className="grid-2">
        <Card title="All groups">
          {!groups ? <Spinner /> : groups.length === 0 ? <Empty title="No groups yet" /> : (
            <div className="list">
              {groups.map((g) => (
                <div key={g.id} className={`list-row ${active?.id === g.id ? 'active' : ''}`} style={{ cursor: 'pointer' }} onClick={() => select(g)}>
                  <div className="list-main">
                    <div className="list-title">{g.name} <Badge tone="editor">{g.kind}</Badge></div>
                    <div className="muted small">{g.leader_email || 'no leader'}{g.meeting_day != null ? ` · ${WEEKDAYS[g.meeting_day]}` : ''}</div>
                  </div>
                  {canEdit && <button className="icon-btn" onClick={(e) => { e.stopPropagation(); setGroupModal(g); }}><Icon.Edit size={16} /></button>}
                  {canEdit && <button className="icon-btn danger" onClick={(e) => { e.stopPropagation(); if (confirm('Delete this group?')) deleteGroup(g.id).then(() => { toast('Deleted.', 'success'); setActive(null); load(); }); }}><Icon.Trash size={16} /></button>}
                </div>
              ))}
            </div>
          )}
        </Card>

        <Card title={active ? `${active.name} — members` : 'Members'}
          actions={active && canEdit ? <Button variant="soft" onClick={() => setAddOpen(true)}><Icon.Plus size={16} /> Add</Button> : undefined}>
          {!active ? <Empty title="Pick a group" /> : members.length === 0 ? <Empty title="No members yet" /> : (
            <div className="list">
              {members.map((m) => (
                <div key={m.id} className="list-row">
                  <Avatar name={m.name || '?'} size={32} />
                  <div className="list-main">
                    <div className="list-title">{m.name || 'Member'} {m.member_role === 'leader' && <Badge tone="leader">leader</Badge>}</div>
                  </div>
                  {canEdit && (
                    <>
                      <button className="icon-btn" title="Toggle leader" onClick={() => toggleRole(m)}><Icon.Edit size={16} /></button>
                      <button className="icon-btn danger" onClick={() => removeGroupMember(m.id).then(() => fetchGroupMembers(active.id).then(setMembers))}><Icon.Trash size={16} /></button>
                    </>
                  )}
                </div>
              ))}
            </div>
          )}
          {active?.description && <p className="muted small">{active.description}</p>}
        </Card>
      </div>

      <Modal open={!!groupModal} title={groupModal?.id ? 'Edit group' : 'New group'} onClose={() => setGroupModal(null)}
        footer={<><Button variant="ghost" onClick={() => setGroupModal(null)}>Cancel</Button><Button disabled={busy || !groupModal?.name?.trim()} onClick={saveGroupModal}>Save</Button></>}>
        <Field label="Name"><input value={groupModal?.name || ''} onChange={(e) => setGroupModal({ ...(groupModal as object), name: e.target.value })} /></Field>
        <Field label="Kind">
          <select value={groupModal?.kind || 'ministry'} onChange={(e) => setGroupModal({ ...(groupModal as object), kind: e.target.value })}>
            <option value="ministry">Ministry</option>
            <option value="cell">Cell group</option>
            <option value="class">Class</option>
            <option value="choir">Choir</option>
            <option value="youth">Youth</option>
          </select>
        </Field>
        <Field label="Leader email"><input value={groupModal?.leader_email || ''} onChange={(e) => setGroupModal({ ...(groupModal as object), leader_email: e.target.value })} /></Field>
        <div className="row-2">
          <Field label="Meets on">
            <select value={groupModal?.meeting_day ?? ''} onChange={(e) => setGroupModal({ ...(groupModal as object), meeting_day: e.target.value === '' ? null : Number(e.target.value) })}>
              <option value="">—</option>
              {WEEKDAYS.map((w, i) => <option key={w} value={i}>{w}</option>)}
            </select>
          </Field>
          <Field label="Time"><input value={groupModal?.meeting_time || ''} onChange={(e) => setGroupModal({ ...(groupModal as object), meeting_time: e.target.value })} placeholder="18:00" /></Field>
        </div>
        <Field label="Location"><input value={groupModal?.location || ''} onChange={(e) => setGroupModal({ ...(groupModal as object), location: e.target.value })} /></Field>
        <Field label="Description"><textarea rows={3} value={groupModal?.description || ''} onChange={(e) => setGroupModal({ ...(groupModal as object), description: e.target.value })} /></Field>
      </Modal>

      <Modal open={addOpen} title="Add member" onClose={() => setAddOpen(false)}
        footer={<><Button variant="ghost" onClick={() => setAddOpen(false)}>Cancel</Button><Button disabled={!pick} onClick={add}>Add</Button></>}>
        <Field label="Person">
          <select value={pick} onChange={(e) => setPick(e.target.value)}>
            <option value="">Choose a member…</option>
            {people.map((p) => <option key={p.id} value={p.id}>{p.full_name || p.email}</option>)}
          </select>
        </Field>
        <Field label="Role">
          <select value={role} onChange={(e) => setRole(e.target.value)}>
            <option value="member">Member</option>
            <option value="leader">Leader</option>
          </select>
        </Field>
      </Modal>
    </div>
  );
}