import { createClient, SupabaseClient } from '@supabase/supabase-js';
import { config, isConfigured } from '../config';

let client: SupabaseClient | null = null;

export function getClient(): SupabaseClient | null {
  if (!isConfigured('supabase')) return null;
  if (!client) {
    client = createClient(config.supabaseUrl, config.supabaseKey, {
      auth: { persistSession: true, autoRefreshToken: true },
      realtime: { params: { eventsPerSecond: 10 } }
    });
  }
  return client;
}

export { isConfigured };