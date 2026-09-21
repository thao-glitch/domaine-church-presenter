import { useEffect, useMemo, useState } from 'react';
import { useAuth } from '../auth';
import { Card, Button, Modal, Field, Badge, Spinner, Empty } from '../components/ui';
import { Icon } from '../components/icons';
import {
  fetchAdminChurches, fetchAllProfiles, deleteChurch, deleteProfile, setProfileRole,
  adminCreateUser, adminCreateChurch,
  type Church, type AdminProfile
} from '../lib/api';
import {
  fetchGlobalContent, saveGlobalContent, deleteGlobalContent, countOf,
  type ContentItem, type ContentKind
} from '../lib/churchlife';
import { ROLES } from '../roles';
import { useToast } from '../components/toast';

const GLOBAL_KINDS: ContentKind[] = ['devotional', 'sermon', 'verse', 'announcement', 'prayer'];

export function PlatformAdminPage() {
  const { isAdmin, enterChurch, profile } = useAuth();
  const toast = useToast();
  const [churches, setChurches] = useState<Church[] | null>(null);
  const [people, setPeople] = useState<AdminProfile[] | null>(null);
  const [global, setGlobal] = useState<ContentItem[] | null>(null);
  const [stats, setStats] = useState<{ sessions: number; prayers: number; giving: number; recordings: number } | null>(null);
  const [gModal, setGModal] = useState<Partial<ContentItem> | null>(null);
  const [busy, setBusy] = useState(false);
  const [churchForm, setChurchForm] = useState(false);
  const [userForm, setUserForm] = useState(false);
  const [nf, setNf] = useState({ name: '', city: '', country: '', description: '', contact_email: '', owner_email: '', owner_name: '' });
  const [uf, setUf] = useState({ email: '', password: '', full_name: '', role: 'Church Admin', church_id: '' });

  function load() {
    fetchAdminChurches().then(setChurches).catch((e) => toast(String(e), 'error'));
    fetchAllProfiles().then(setPeople).catch((e) => toast(String(e), 'error'));
    fetchGlobalContent().then(setGlobal).catch(() => setGlobal([]));
    Promise.all([countOf('sessions'), countOf('prayers'), countOf('giving'), countOf('recordings')])
      .then(([sessions, prayers, giving, recordings]) => setStats({ sessions, prayers, giving, recordings }))
      .catch(() => setStats({ sessions: 0, prayers: 0, giving: 0, recordings: 0 }));
  }

  useEffect(() => { if (isAdmin) load(); /* eslint-disable-next-line react-hooks/exhaustive-deps */ }, [isAdmin]);

  const byChurch = useMemo(() => {
    const map = new Map<string, number>();
    (people || []).forEach((p) => {
      if (p.church_id) map.set(p.church_id, (map.get(p.church_id) || 0) + 1);
    });
    return map;
  }, [people]);

  const churchName = (id: string | null) => {
    if (!id) return '—';
    return (churches || []).find((c) => c.id === id)?.name || 'Unknown church';
  };

  async function saveGlobal() {
    if (!gModal || !gModal.title?.trim()) return;
    setBusy(true);
    try {
      await saveGlobalContent({ ...gModal, title: gModal.title.trim(), created_by: profile?.email || null });
      toast('Global content published to every church.', 'success');
      setGModal(null); load();
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
    finally { setBusy(false); }
  }

  async function removeChurch(c: Church) {
    if (!confirm(`Delete "${c.name}"? Its members' links stay, but the church app disappears.`)) return;
    try {
      await deleteChurch(c.id);
      toast(`${c.name} deleted.`, 'success');
      load();
    } catch (e) {
      toast(e instanceof Error ? e.message : String(e), 'error');
    }
  }

  async function changeRole(p: AdminProfile, next: string) {
    try {
      await setProfileRole(p.id, next);
      toast(`${p.full_name || p.email} is now ${next}.`, 'success');
      setPeople((prev) => (prev || []).map((x) => (x.id === p.id ? { ...x, role: next } : x)));
    } catch (e) {
      toast(e instanceof Error ? e.message : String(e), 'error');
    }
  }

  async function removeUser(p: AdminProfile) {
    if (!confirm(`Remove the account for ${p.email}? They will no longer be able to sign in.`)) return;
    try {
      await deleteProfile(p.id);
      toast(`${p.email} removed.`, 'success');
      setPeople((prev) => (prev || []).filter((x) => x.id !== p.id));
    } catch (e) {
      toast(e instanceof Error ? e.message : String(e), 'error');
    }
  }

  async function createChurch() {
    if (!nf.name.trim()) return;
    setBusy(true);
    try {
      await adminCreateChurch(nf);
      toast(`"${nf.name.trim()}" registered.`, 'success');
      setChurchForm(false); setNf({ name: '', city: '', country: '', description: '', contact_email: '', owner_email: '', owner_name: '' });
      load();
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
    finally { setBusy(false); }
  }

  async function createUser() {
    if (!uf.email.trim() || !uf.full_name.trim() || !uf.password || !uf.church_id) return;
    setBusy(true);
    try {
      await adminCreateUser(uf);
      toast(`Account created for ${uf.email.trim()} as ${uf.role}.`, 'success');
      setUserForm(false); setUf({ email: '', password: '', full_name: '', role: 'Church Admin', church_id: '' });
      load();
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
    finally { setBusy(false); }
  }

  if (!isAdmin) {
    return <Empty title="Platform admins only" sub="This area is restricted to the overall administrator." />;
  }

  return (
    <div className="stack">
      <Card title="All church apps">
        <div className="stat-row">
          <div className="stat"><div className="stat-value">{churches ? churches.length : '…'}</div><div className="stat-label">churches</div></div>
          <div className="stat"><div className="stat-value">{people ? people.length : '…'}</div><div className="stat-label">accounts</div></div>
          <div className="stat"><div className="stat-value">{stats ? stats.sessions : '…'}</div><div className="stat-label">sessions</div></div>
          <div className="stat"><div className="stat-value">{stats ? stats.prayers : '…'}</div><div className="stat-label">prayers</div></div>
          <div className="stat"><div className="stat-value">{stats ? stats.giving : '…'}</div><div className="stat-label">giving records</div></div>
          <div className="stat"><div className="stat-value">{stats ? stats.recordings : '…'}</div><div className="stat-label">recordings</div></div>
        </div>
      </Card>

      <Card title="Global content library"
        actions={<Button onClick={() => setGModal({ kind: 'devotional', state: 'published', channels: ['app'] })}><Icon.Plus size={16} /> Publish to all churches</Button>}>
        <p className="muted small">Devotionals, verses and sermon series you publish here appear in every church's feed automatically.</p>
        {!global ? <Spinner /> : global.length === 0 ? <Empty title="No global content yet" /> : (
          <div className="list">
            {global.map((c) => (
              <div key={c.id} className="list-row">
                <div className="list-main">
                  <div className="list-title">{c.title} <Badge tone="leader">{c.kind}</Badge>{c.state !== 'published' && <Badge tone="warn">{c.state}</Badge>}</div>
                  {c.body && <div className="muted small content-body-preview">{c.body}</div>}
                </div>
                <div className="row-actions">
                  <button className="icon-btn" onClick={() => setGModal(c)}><Icon.Edit size={16} /></button>
                  <button className="icon-btn danger" onClick={() => confirm('Delete this global content?') && deleteGlobalContent(c.id).then(() => { toast('Deleted.', 'success'); load(); })}><Icon.Trash size={16} /></button>
                </div>
              </div>
            ))}
          </div>
        )}
      </Card>

      <Card title="Churches"
        actions={<Button onClick={() => setChurchForm(true)}><Icon.Plus size={16} /> Register church</Button>}>
        {!churches ? <Spinner /> : churches.length === 0 ? (
          <Empty title="No churches yet" sub="Churches appear here as they register from the sign-up screen." />
        ) : (
          <div className="list">
            {churches.map((c) => (
              <div key={c.id} className="list-row">
                <div className="list-main">
                  <div className="list-title">{c.name} {c.city ? <span className="muted small">· {c.city}</span> : null}</div>
                  <div className="muted small">
                    {byChurch.get(c.id) || 0} account{(byChurch.get(c.id) || 0) === 1 ? '' : 's'} ·
                    {' '}owner {c.owner_name || c.owner_email || '—'}
                  </div>
                </div>
                <Button variant="soft" onClick={() => enterChurch(c.id)}>Manage this church</Button>
                <Button variant="ghost" onClick={() => removeChurch(c)}>Delete</Button>
              </div>
            ))}
          </div>
        )}
      </Card>

      <Card title="All accounts"
        actions={<Button onClick={() => setUserForm(true)}><Icon.Plus size={16} /> Add account</Button>}>
        {!people ? <Spinner /> : people.length === 0 ? (
          <Empty title="No accounts yet" sub="People appear here after they create an account." />
        ) : (
          <div className="list">
            {people.map((p) => (
              <div key={p.id} className="list-row">
                <div className="list-main">
                  <div className="list-title">{p.full_name || p.email}</div>
                  <div className="muted small">{p.email} · {churchName(p.church_id)}</div>
                </div>
                {p.email === profile?.email ? (
                  <Badge tone="success">{p.role} (you)</Badge>
                ) : (
                  <>
                    <select className="role-select" defaultValue={p.role} onChange={(e) => changeRole(p, e.target.value)}>
                      {ROLES.map((r) => <option key={r.name} value={r.name}>{r.name}</option>)}
                    </select>
                    <Button variant="ghost" onClick={() => removeUser(p)}>Remove</Button>
                  </>
                )}
              </div>
            ))}
          </div>
        )}
      </Card>

      <Modal open={!!gModal} title={gModal?.id ? 'Edit global content' : 'Publish global content'} onClose={() => setGModal(null)}
        footer={<><Button variant="ghost" onClick={() => setGModal(null)}>Cancel</Button><Button disabled={busy || !gModal?.title?.trim()} onClick={saveGlobal}>{busy ? 'Saving…' : 'Publish'}</Button></>}>
        <Field label="Type">
          <select value={gModal?.kind || 'devotional'} onChange={(e) => setGModal({ ...(gModal as object), kind: e.target.value as ContentKind })}>
            {GLOBAL_KINDS.map((k) => <option key={k} value={k}>{k}</option>)}
          </select>
        </Field>
        <Field label="Title"><input value={gModal?.title || ''} onChange={(e) => setGModal({ ...(gModal as object), title: e.target.value })} /></Field>
        <Field label="Message"><textarea rows={5} value={gModal?.body || ''} onChange={(e) => setGModal({ ...(gModal as object), body: e.target.value })} /></Field>
        <Field label="Link (optional)"><input value={gModal?.link_url || ''} onChange={(e) => setGModal({ ...(gModal as object), link_url: e.target.value })} /></Field>
      </Modal>

      <Modal open={churchForm} title="Register a church" onClose={() => setChurchForm(false)}
        footer={<><Button variant="ghost" onClick={() => setChurchForm(false)}>Cancel</Button><Button disabled={busy || !nf.name.trim()} onClick={createChurch}>{busy ? 'Saving…' : 'Register church'}</Button></>}>
        <p className="muted small">After registering, add its first admin from the accounts card.</p>
        <Field label="Church name"><input value={nf.name} onChange={(e) => setNf({ ...nf, name: e.target.value })} placeholder="e.g. Good Shepherd Assembly" /></Field>
        <div className="grid-2">
          <Field label="City"><input value={nf.city} onChange={(e) => setNf({ ...nf, city: e.target.value })} placeholder="optional" /></Field>
          <Field label="Country"><input value={nf.country} onChange={(e) => setNf({ ...nf, country: e.target.value })} placeholder="optional" /></Field>
        </div>
        <Field label="About"><textarea rows={3} value={nf.description} onChange={(e) => setNf({ ...nf, description: e.target.value })} placeholder="optional" /></Field>
        <div className="grid-2">
          <Field label="Contact email"><input value={nf.contact_email} onChange={(e) => setNf({ ...nf, contact_email: e.target.value })} placeholder="optional" /></Field>
          <Field label="Owner email"><input value={nf.owner_email} onChange={(e) => setNf({ ...nf, owner_email: e.target.value })} placeholder="optional" /></Field>
        </div>
        <Field label="Owner name"><input value={nf.owner_name} onChange={(e) => setNf({ ...nf, owner_name: e.target.value })} placeholder="optional" /></Field>
      </Modal>

      <Modal open={userForm} title="Add an account" onClose={() => setUserForm(false)}
        footer={<><Button variant="ghost" onClick={() => setUserForm(false)}>Cancel</Button><Button disabled={busy || !uf.email.trim() || !uf.full_name.trim() || !uf.password || !uf.church_id} onClick={createUser}>{busy ? 'Saving…' : 'Create account'}</Button></>}>
        <p className="muted small">Creates a fully activated sign-in (no confirmation email needed).</p>
        <Field label="Full name"><input value={uf.full_name} onChange={(e) => setUf({ ...uf, full_name: e.target.value })} /></Field>
        <Field label="Email"><input type="email" value={uf.email} onChange={(e) => setUf({ ...uf, email: e.target.value })} /></Field>
        <Field label="Password"><input type="password" value={uf.password} onChange={(e) => setUf({ ...uf, password: e.target.value })} /></Field>
        <div className="grid-2">
          <Field label="Church">
            <select value={uf.church_id} onChange={(e) => setUf({ ...uf, church_id: e.target.value })}>
              <option value="">Select church…</option>
              {(churches || []).map((c) => <option key={c.id} value={c.id}>{c.name}</option>)}
            </select>
          </Field>
          <Field label="Role">
            <select value={uf.role} onChange={(e) => setUf({ ...uf, role: e.target.value })}>
              {ROLES.map((r) => <option key={r.name} value={r.name}>{r.name}</option>)}
            </select>
          </Field>
        </div>
      </Modal>
    </div>
  );
}