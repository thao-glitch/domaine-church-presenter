import { createClient } from '@supabase/supabase-js';

// Helper used outside of this repo's type system stays permissive.
export type Sb = ReturnType<typeof createClient>;
export type { SupabaseClient } from '@supabase/supabase-js';