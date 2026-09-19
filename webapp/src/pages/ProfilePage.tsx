import { useEffect, useState } from 'react';
import { useAuth } from '../auth';
import { Card, Button, Field, Avatar, RoleBadge } from '../components/ui';
import { getClient } from '../lib/supabase';
import { fetchChurches, type Church } from '../lib/api';
import { useToast } from '../components/toast';

export function ProfilePage() {
  const { user, profile, churchName, churchId, refreshProfile } = useAuth();
  const toast = useToast();
  const [name, setName] = useState(profile?.full_name || user?.email?.split('@')[0] || '');
  const [churches, setChurches] = useState<Church[]>([]);
  const [pick, setPick] = useState('');
  const [busy, setBusy] = useState(false);

  useEffect(() => {
    fetchChurches().then(setChurches).catch(() => setChurches([]));
  }, []);

  async function save() {
    const sb = getClient();
    if (!sb || !profile) return;
    setBusy(true);
    try {
      const { error } = await sb.from('profiles')
        .update({ full_name: name.trim() }).eq('id', profile.id);
      if (error) throw new Error(error.message);
      afterSave();
    } catch (e) {
      toast(e instanceof Error ? e.message : String(e), 'error');
      setBusy(false);
    }
  }

  async function joinChurch() {
    const sb = getClient();
    if (!sb || !profile || !pick) return;
    setBusy(true);
    try {
      const { error } = await sb.from('profiles').update({ church_id: pick }).eq('id', profile.id);
      if (error) throw new Error(error.message);
      toast('You are now linked to your church.', 'success');
      setBusy(false);
      refreshProfile();
    } catch (e) {
      toast(e instanceof Error ? e.message : String(e), 'error');
      setBusy(false);
    }
  }

  function afterSave() {
    toast('Profile updated.', 'success');
    setBusy(false);
    refreshProfile();
  }

  return (
    <div className="stack">
      <Card title="My profile">
        <div className="profile-top">
          <Avatar name={profile?.full_name || user?.email || '?'} size={64} />
          <div>
            <div className="profile-name">{profile?.full_name || user?.email}</div>
            <div className="muted">{user?.email}</div>
            <div className="spacer-s" />
            <RoleBadge role={profile?.role || 'Member'} />
            {churchName && <div className="muted small">at {churchName}</div>}
          </div>
        </div>
      </Card>

      {!churchId && (
        <Card title="Choose your church">
          <p className="muted">Your account isn't linked to a church yet. Pick the church you attend so you see its members, services, media and sessions.</p>
          <Field label="Your church">
            <select value={pick} onChange={(e) => setPick(e.target.value)}>
              <option value="">Choose a church…</option>
              {churches.map((c) => (
                <option key={c.id} value={c.id}>{c.name}{c.city ? ` · ${c.city}` : ''}</option>
              ))}
            </select>
          </Field>
          <div className="card-actions">
            <Button onClick={joinChurch} disabled={busy || !pick}>{busy ? 'Saving…' : 'Link my church'}</Button>
          </div>
        </Card>
      )}

      <Card title="Personal details">
        {!profile && (
          <p className="muted">Your account is not attached to a role profile yet.
            <Button variant="soft" className="ms" onClick={refreshProfile}>Attach now</Button>
          </p>
        )}
        {profile && (
          <>
            <Field label="Full name">
              <input value={name} onChange={(e) => setName(e.target.value)} />
            </Field>
            <Field label="Role assigned by church leadership">
              <input value={profile.role || 'Member'} disabled />
            </Field>
            <div className="card-actions">
              <Button onClick={save} disabled={busy}>{busy ? 'Saving…' : 'Save changes'}</Button>
            </div>
          </>
        )}
      </Card>
    </div>
  );
}