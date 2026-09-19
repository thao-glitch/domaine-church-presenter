import { useState } from 'react';
import { useAuth } from '../auth';
import { Card, Button, Field, Avatar, RoleBadge } from '../components/ui';
import { getClient } from '../lib/supabase';
import { useToast } from '../components/toast';

export function ProfilePage() {
  const { user, profile, refreshProfile } = useAuth();
  const toast = useToast();
  const [name, setName] = useState(profile?.full_name || user?.email?.split('@')[0] || '');
  const [busy, setBusy] = useState(false);

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
          </div>
        </div>
      </Card>

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