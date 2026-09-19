// Role levels shared with the desktop presenter (rank 0 = highest).
export interface RoleDef {
  name: string;
  tier: 'leader' | 'editor' | 'member';
  canEdit: boolean;
  canPresent: boolean;
  canSchedule: boolean;
}

export const ROLES: RoleDef[] = [
  { name: 'Bishop', tier: 'leader', canEdit: true, canPresent: true, canSchedule: true },
  { name: 'Senior Pastor', tier: 'leader', canEdit: true, canPresent: true, canSchedule: true },
  { name: 'Pastor', tier: 'leader', canEdit: true, canPresent: true, canSchedule: true },
  { name: 'Assistant Pastor', tier: 'leader', canEdit: true, canPresent: true, canSchedule: true },
  { name: 'Elder', tier: 'leader', canEdit: true, canPresent: true, canSchedule: true },
  { name: 'Church Admin', tier: 'leader', canEdit: true, canPresent: true, canSchedule: true },
  { name: 'Deacon', tier: 'editor', canEdit: true, canPresent: false, canSchedule: true },
  { name: 'Deaconess', tier: 'editor', canEdit: true, canPresent: false, canSchedule: true },
  { name: 'Evangelist', tier: 'editor', canEdit: true, canPresent: false, canSchedule: true },
  { name: 'Minister', tier: 'editor', canEdit: true, canPresent: false, canSchedule: true },
  { name: 'Worship Leader', tier: 'editor', canEdit: true, canPresent: false, canSchedule: true },
  { name: 'Choir', tier: 'editor', canEdit: true, canPresent: false, canSchedule: false },
  { name: 'Usher', tier: 'member', canEdit: false, canPresent: false, canSchedule: false },
  { name: 'Greeter', tier: 'member', canEdit: false, canPresent: false, canSchedule: false },
  { name: 'Media', tier: 'member', canEdit: false, canPresent: true, canSchedule: false },
  { name: 'Youth Leader', tier: 'editor', canEdit: true, canPresent: false, canSchedule: true },
  { name: 'Member', tier: 'member', canEdit: false, canPresent: false, canSchedule: false }
];

export function roleIndex(role: string): number {
  return ROLES.findIndex((r) => r.name.toLowerCase() === String(role || '').toLowerCase());
}

export function roleDef(role: string): RoleDef {
  const i = roleIndex(role);
  return i >= 0 ? ROLES[i] : ROLES[ROLES.length - 1];
}

export const WEEKDAYS = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday'];

/** Service types available church-wide (research-backed list). */
export const SERVICE_TYPES = [
  'Sunday Worship Service', 'Sunday School', 'Evening Worship Service', 'Saturday Service',
  'Midweek Service', 'Youth Service', 'Children Church', 'Bible Study', 'Prayer Meeting',
  'Covenant Hour of Prayer', 'Thanksgiving Service', 'Harvest Service', 'New Year Service',
  'Easter Service', 'Good Friday Service', 'Christmas Service', 'Ash Wednesday Service',
  'Maundy Thursday Service', 'Holy Communion Service', 'Baptism Service', 'Confirmation Service',
  'Ordination Service', 'Wedding Service', 'Funeral Service', 'Dedication Service',
  'Healing Service', 'Revival Service', 'Crusade', 'Outreach', 'Evangelism', 'Camp Meeting',
  'Discipleship Class', 'Catechism Class', 'Leadership Conference', 'Community Meeting',
  'Men\'s Ministry Meeting', 'Women\'s Ministry Meeting', 'Fellowship Meeting', 'Cell Group',
  'Choir Practice', 'Media Ministry', 'Ushering Ministry', 'Visitation', 'Special Program'
];

/** Event categories. */
export const EVENT_CATEGORIES = [
  'Special Service', 'Anniversary', 'Conference', 'Seminar', 'Music Concert', 'Convention',
  'Outreach', 'Fellowship', 'Retreat', 'Fundraiser', 'Memorial', 'Youth Program', 'Other'
];