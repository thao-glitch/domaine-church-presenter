export const Icon = {
  Home: (p: Svg) => <Svg {...p}><path d="M3 10.5 12 3l9 7.5V21h-6v-6h-6v6H3z" /></Svg>,
  Users: (p: Svg) => <Svg {...p}><circle cx="9" cy="8" r="3.4" /><path d="M2.5 20c.6-3.6 3.2-5.5 6.5-5.5s5.9 1.9 6.5 5.5M16 5.2a3.4 3.4 0 0 1 0 6M21.5 20c-.4-2.6-1.8-4.4-4-5.2" /></Svg>,
  Calendar: (p: Svg) => <Svg {...p}><rect x="3.5" y="5" width="17" height="16" rx="2" /><path d="M3.5 10h17M8 3v4M16 3v4" /></Svg>,
  Upload: (p: Svg) => <Svg {...p}><path d="M12 16V4m0 0-4 4m4-4 4 4M4 16v4h16v-4" /></Svg>,
  Chat: (p: Svg) => <Svg {...p}><path d="M21 12a8 8 0 0 1-11.6 7.1L3 21l1.9-6.4A8 8 0 1 1 21 12Z" /><path d="M9 10h6M9 14h3" /></Svg>,
  Video: (p: Svg) => <Svg {...p}><rect x="3" y="6" width="11" height="12" rx="2" /><path d="m14 10 7-4v12l-7-4" /></Svg>,
  Stage: (p: Svg) => <Svg {...p}><rect x="3" y="4" width="18" height="11" rx="1.5" /><path d="M8 19h8M12 15v4" /></Svg>,
  Logout: (p: Svg) => <Svg {...p}><path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4M16 17l5-5-5-5M21 12H9" /></Svg>,
  Menu: (p: Svg) => <Svg {...p}><path d="M4 6h16M4 12h16M4 18h16" /></Svg>,
  Close: (p: Svg) => <Svg {...p}><path d="M6 6l12 12M18 6 6 18" /></Svg>,
  Plus: (p: Svg) => <Svg {...p}><path d="M12 5v14M5 12h14" /></Svg>,
  Edit: (p: Svg) => <Svg {...p}><path d="M4 20h4L20 8l-4-4L4 16v4Z" /></Svg>,
  Trash: (p: Svg) => <Svg {...p}><path d="M4 7h16M9 7V4h6v3m-8 0 1 13h8l1-13" /></Svg>,
  Play: (p: Svg) => <Svg {...p}><path d="M7 4l13 8-13 8z" /></Svg>,
  Search: (p: Svg) => <Svg {...p}><circle cx="11" cy="11" r="6" /><path d="m18 18 3 3" /></Svg>,
  Mic: (p: Svg) => <Svg {...p}><rect x="9" y="2.5" width="6" height="11" rx="3" /><path d="M5 11a7 7 0 0 0 14 0M12 18v3.5" /></Svg>,
  CamOff: (p: Svg) => <Svg {...p}><rect x="3" y="6" width="11" height="12" rx="2" /><path d="m14 10 7-4v12l-7-4M3 3l18 18" /></Svg>,
  Send: (p: Svg) => <Svg {...p}><path d="m3 11 18-8-8 18-2.5-7.5L3 11Z" /></Svg>,
  Stop: (p: Svg) => <Svg {...p}><rect x="6" y="6" width="12" height="12" rx="2" /></Svg>,
  Church: (p: Svg) => <Svg {...p}><path d="M12 3 4 8v13h6v-6h4v6h6V8l-8-5Z" /><path d="M10 8h4M12 6.5V9" /></Svg>,
  CamSwitch: (p: Svg) => <Svg {...p}><path d="M7 8h10l3 3v3l-8 5V8Z" /><path d="M9 8 8 5h5l-1 3" /></Svg>
};

interface Svg { size?: number; className?: string; }

function Svg({ size = 20, className, children }: Svg & { children: React.ReactNode }) {
  return (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none"
      stroke="currentColor" strokeWidth="1.8" strokeLinecap="round"
      strokeLinejoin="round" className={className} aria-hidden="true">
      {children}
    </svg>
  );
}