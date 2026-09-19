// ---------------------------------------------------------------------
// App configuration.
// The Supabase URL + anon key are public by design (client-side SDK).
// Defaults ship in code; users can paste their own values in the Setup
// screen, which are persisted to localStorage so the published site can
// be pointed at any Supabase project without a rebuild.
// ---------------------------------------------------------------------

export const APP_NAME = 'Domaine Church';

export interface AppConfig {
  name: string;
  supabaseUrl: string;
  supabaseKey: string;
  livekitUrl: string;
  /** Supabase Edge Function that mints LiveKit tokens (see supabase/functions/livekit-token) */
  livekitTokenUrl: string;
}

const STORAGE_KEY = 'dc-config';

const defaults: AppConfig = {
  name: APP_NAME,
  supabaseUrl: 'https://YOUR-PROJECT.supabase.co',
  supabaseKey: 'YOUR-ANON-KEY',
  livekitUrl: 'wss://your-project.livekit.cloud',
  livekitTokenUrl: 'https://YOUR-PROJECT.supabase.co/functions/v1/livekit-token'
};

export function resolveConfig(): AppConfig {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (raw) return { ...defaults, ...JSON.parse(raw) };
  } catch {
    /* ignore */
  }
  return defaults;
}

export function saveConfig(cfg: AppConfig) {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(cfg));
}

export const config: AppConfig = resolveConfig();

export function isConfigured(part: 'supabase' | 'livekit'): boolean {
  const c = resolveConfig();
  if (part === 'supabase') {
    return (
      c.supabaseUrl.startsWith('https://') &&
      !c.supabaseUrl.includes('YOUR-PROJECT') &&
      c.supabaseKey !== 'YOUR-ANON-KEY' &&
      c.supabaseKey.length > 10
    );
  }
  return (
    c.livekitUrl.replace('wss://', 'https://').startsWith('https://') &&
    !c.livekitUrl.includes('your-project') &&
    c.livekitTokenUrl.replace('https://', '').replace('/functions/v1/livekit-token', '').length > 10 &&
    !c.livekitTokenUrl.includes('YOUR-PROJECT')
  );
}