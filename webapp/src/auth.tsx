import { createContext, useContext, useEffect, useState } from 'react';
import type { ReactNode } from 'react';
import type { User, Session } from '@supabase/supabase-js';
import { getClient, isConfigured } from './lib/supabase';
import { roleDef, ROLES } from './roles';
import { setChurchScope, fetchChurch } from './lib/api';

export interface Profile {
  id: string;
  email: string;
  full_name: string | null;
  role: string;
  church_id: string | null;
}

interface AuthState {
  configuring: boolean;
  user: User | null;
  profile: Profile | null;
  churchName: string | null;
  churchId: string | null;
  roleLabel: string;
  canEdit: boolean;
  canPresent: boolean;
  canSchedule: boolean;
  ready: boolean;
  signIn: (email: string, password: string) => Promise<string | null>;
  signUp: (email: string, password: string, fullName: string, churchId: string) => Promise<string | null>;
  createProfile: () => Promise<void>;
  signOut: () => Promise<void>;
  refreshProfile: () => Promise<void>;
}

const AuthCtx = createContext<AuthState>(null as unknown as AuthState);

export function useAuth() {
  return useContext(AuthCtx);
}

export function AuthProvider({ children }: { children: ReactNode }) {
  const [configuring] = useState(!isConfigured('supabase'));
  const [session, setSession] = useState<Session | null>(null);
  const [profile, setProfile] = useState<Profile | null>(null);
  const [churchName, setChurchName] = useState<string | null>(null);
  const [ready, setReady] = useState(false);
  const sb = getClient();

  const loadProfile = async (sess: Session | null) => {
    if (!sess || !sb) { setProfile(null); setChurchName(null); setChurchScope(null); return; }
    const email = sess.user.email || '';
    const { data } = await sb.from('profiles').select('*').eq('email', email).maybeSingle();
    const prof = (data as Profile | undefined) ?? null;
    setProfile(prof);
    setChurchScope(prof?.church_id ?? null);
    if (prof?.church_id) {
      fetchChurch(prof.church_id).then((c) => setChurchName(c?.name ?? null)).catch(() => setChurchName(null));
    } else {
      setChurchName(null);
    }
  };

  useEffect(() => {
    if (!sb) { setReady(true); return; }
    sb.auth.getSession().then(({ data }) => {
      setSession(data.session);
      loadProfile(data.session).then(() => setReady(true));
    });
    const { data: sub } = sb.auth.onAuthStateChange((_event, sess) => {
      setSession(sess);
      loadProfile(sess);
    });
    return () => sub.subscription.unsubscribe();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  async function signIn(email: string, password: string): Promise<string | null> {
    if (!sb) return 'Not configured.';
    const { error } = await sb.auth.signInWithPassword({ email, password });
    return error ? error.message : null;
  }

  async function signUp(email: string, password: string, fullName: string, churchId: string): Promise<string | null> {
    if (!sb) return 'Not configured.';
    const { error } = await sb.auth.signUp({ email, password });
    if (error) return error.message;
    const u = sb.auth.getUser();
    const usr = (await u).data.user;
    if (usr) {
      await sb.from('profiles').upsert({
        id: usr.id, email, full_name: fullName, role: 'Member', church_id: churchId
      }, { onConflict: 'id' });
    }
    return null;
  }

  async function createProfile() {
    if (!sb || !session?.user) return;
    await sb.from('profiles').upsert({
      id: session.user.id,
      email: session.user.email || '',
      full_name: session.user.user_metadata?.full_name || '',
      role: 'Member'
    }, { onConflict: 'id' });
    await loadProfile(session);
  }

  async function signOut() {
    await sb?.auth.signOut();
  }

  const rd = profile ? roleDef(profile.role) : ROLES[ROLES.length - 1];

  return (
    <AuthCtx.Provider value={{
      configuring,
      user: session?.user ?? null,
      profile,
      churchName,
      churchId: profile?.church_id ?? null,
      roleLabel: (profile?.role || 'Visitor'),
      canEdit: rd.canEdit,
      canPresent: rd.canPresent,
      canSchedule: rd.canSchedule,
      ready,
      signIn,
      signUp,
      createProfile,
      signOut,
      refreshProfile: () => loadProfile(session)
    }}>
      {children}
    </AuthCtx.Provider>
  );
}