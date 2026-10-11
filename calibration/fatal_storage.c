/* Bounded PC loader-zero cells. Native access widths and lifetimes supported;
 * original spellings, signedness, aggregate identity and TU remain unproved.
 * These names are the current fatal consumer's measured address identities.
 */
typedef void *FatalWindow;

FatalWindow g_fatal_owner_window;
/* Native current fullscreen state; also suppresses the fatal MessageBox. */
unsigned char g_fatal_suppress_dialog;
/* Process-exit reentrancy guard shared with the native normal-exit service. */
unsigned char g_fatal_entered;
