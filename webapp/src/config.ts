// ---------------------------------------------------------------------
// App configuration. The Supabase URL + anon key are public by design
// (client-side SDK). Values are shipped directly in the build so the
// deployed apps work with no setup screen.
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

const defaults: AppConfig = {
  name: 'Domaine Church',
  supabaseUrl: 'https://czkgvjymloodnspchabl.supabase.co',
  supabaseKey: 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImN6a2d2anltbG9vZG5zcGNoYWJsIiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODk3OTU0MzQsImV4cCI6MjEwNTM3MTQzNH0.3-7MS7p9K9qUa2LRyN4Jo8BX3Dpw_EVbeBLl6Tq8EZE',
  livekitUrl: 'wss://church-domaine-8r74eapn.livekit.cloud',
  livekitTokenUrl: 'https://czkgvjymloodnspchabl.supabase.co/functions/v1/livekit-token'
};

export function resolveConfig(): AppConfig {
  return defaults;
}

export const config: AppConfig = defaults;

export function isConfigured(_part?: 'supabase' | 'livekit'): boolean {
  return true;
}